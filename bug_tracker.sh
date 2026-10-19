#!/usr/bin/env bash

API_BASE="https://dhjustchillinlgtm.pythonanywhere.com/api"
KEY=""

API_RESPONSE=""
HTTP_STATUS=""
MENU_CHOICE=""
INPUT_VALUE=""

for dependency in curl jq dialog; do
  if ! command -v "$dependency" >/dev/null 2>&1; then
    printf 'Error: required command not found: %s\n' "$dependency" >&2
    clear
    exit 1
  fi
done

show_message() {
  dialog --title "${2:-Bug Tracker}" --msgbox "$1" 0 0
}

show_api_error() {
  local details message

  if [[ -n "$API_RESPONSE" ]]; then
    details=$(jq -r '
      if type == "object" then
        to_entries | map("\(.key): \(.value | tostring)") | join("\n")
      else
        tostring
      end
    ' <<< "$API_RESPONSE" 2>/dev/null)

    if [[ -z "$details" ]]; then
      details="$API_RESPONSE"
    fi
  else
    details="The server returned no response body."
  fi

  printf -v message 'Request failed (HTTP %s).\n\n%s' "$HTTP_STATUS" "$details"
  show_message "$message" "API Error"
}

api_request() {
  local method="$1"
  local endpoint="$2"
  local body="${3-}"
  local has_body=0
  local response_file
  local curl_rc
  local -a curl_args

  if [[ $# -ge 3 ]]; then
    has_body=1
  fi

  response_file=$(mktemp) || {
    HTTP_STATUS="client error"
    API_RESPONSE="Could not create a temporary response file."
    show_api_error
    return 1
  }

  curl_args=(
    -sS
    -o "$response_file"
    -w '%{http_code}'
    -X "$method"
  )

  if [[ -n "$KEY" ]]; then
    curl_args+=(-H "Authorization: Token $KEY")
  fi

  if (( has_body )); then
    curl_args+=(
      -H "Content-Type: application/json"
      --data "$body"
    )
  fi

  if HTTP_STATUS=$(curl "${curl_args[@]}" "$API_BASE$endpoint"); then
    API_RESPONSE=$(cat "$response_file")
    rm -f "$response_file"

    if [[ "$HTTP_STATUS" =~ ^2[0-9][0-9]$ ]]; then
      return 0
    fi

    show_api_error
    return 1
  else
    curl_rc=$?
    API_RESPONSE=$(cat "$response_file")
    rm -f "$response_file"
    HTTP_STATUS="transport error (curl exit $curl_rc)"
    show_api_error
    return 1
  fi
}

dialog_menu() {
  local title="$1"
  local prompt="$2"
  shift 2

  MENU_CHOICE=$(dialog \
    --clear \
    --title "$title" \
    --menu "$prompt" \
    0 0 0 \
    "$@" \
    3>&1 1>&2 2>&3)

  return $?
}

prompt_input() {
  local title="$1"
  local prompt="$2"
  local default="${3-}"

  INPUT_VALUE=$(dialog \
    --clear \
    --title "$title" \
    --inputbox "$prompt" \
    10 70 "$default" \
    3>&1 1>&2 2>&3)

  return $?
}

prompt_secret() {
  local title="$1"
  local prompt="$2"

  INPUT_VALUE=$(dialog \
    --clear \
    --title "$title" \
    --passwordbox "$prompt" \
    10 70 \
    3>&1 1>&2 2>&3)

  return $?
}

set_key_from_response() {
  local new_key

  new_key=$(jq -r '.key // empty' <<< "$API_RESPONSE" 2>/dev/null)
  if [[ -z "$new_key" ]]; then
    API_RESPONSE="The server response did not contain an authentication key."
    HTTP_STATUS="invalid response"
    show_api_error
    return 1
  fi

  KEY="$new_key"
  return 0
}

login() {
  local username password payload

  if ! prompt_input "Log In" "Username:"; then
    return
  fi
  username="$INPUT_VALUE"

  if ! prompt_secret "Log In" "Password:"; then
    return
  fi
  password="$INPUT_VALUE"

  payload=$(jq -n \
    --arg username "$username" \
    --arg password "$password" \
    '{username: $username, password: $password}')

  if api_request POST "/auth/login/" "$payload"; then
    if set_key_from_response; then
      show_message "Login successful." "Bug Tracker"
    fi
  fi
}

register_account() {
  local username password password_confirm payload

  if ! prompt_input "Create Account" "Username:"; then
    return
  fi
  username="$INPUT_VALUE"

  if ! prompt_secret "Create Account" "Password:"; then
    return
  fi
  password="$INPUT_VALUE"

  if ! prompt_secret "Create Account" "Confirm password:"; then
    return
  fi
  password_confirm="$INPUT_VALUE"

  payload=$(jq -n \
    --arg username "$username" \
    --arg password "$password" \
    --arg password_confirm "$password_confirm" \
    '{
      username: $username,
      password: $password,
      password_confirm: $password_confirm
    }')

  if api_request POST "/auth/register/" "$payload"; then
    if set_key_from_response; then
      show_message "Account created and logged in." "Bug Tracker"
    fi
  fi
}

urlencode() {
  jq -rn --arg value "$1" '$value | @uri'
}

append_query_param() {
  local current_endpoint="$1"
  local name="$2"
  local value="$3"
  local separator="?"

  if [[ "$current_endpoint" == *"?"* ]]; then
    separator="&"
  fi

  printf '%s%s%s=%s' \
    "$current_endpoint" \
    "$separator" \
    "$(urlencode "$name")" \
    "$(urlencode "$value")"
}

choose_status_filter() {
  dialog_menu "Search Tickets" "Filter by status:" \
    all "Any status" \
    open "Open" \
    in_progress "In progress" \
    resolved "Resolved" \
    closed "Closed" \
    reopened "Reopened"
}

choose_priority_filter() {
  dialog_menu "Search Tickets" "Filter by priority:" \
    all "Any priority" \
    low "Low" \
    normal "Normal" \
    high "High" \
    critical "Critical"
}

search_tickets() {
  local endpoint="/tickets/"
  local status priority category term ordering

  if ! choose_status_filter; then
    return
  fi
  status="$MENU_CHOICE"

  if ! choose_priority_filter; then
    return
  fi
  priority="$MENU_CHOICE"

  if ! prompt_input "Search Tickets" "Category (leave blank for any):"; then
    return
  fi
  category="$INPUT_VALUE"

  if ! prompt_input "Search Tickets" "Search text (leave blank for any):"; then
    return
  fi
  term="$INPUT_VALUE"

  if ! dialog_menu "Search Tickets" "Ordering:" \
    all "Default ordering" \
    newest "-created_at"; then
    return
  fi
  ordering="$MENU_CHOICE"

  if [[ "$status" != "all" ]]; then
    endpoint=$(append_query_param "$endpoint" "status" "$status")
  fi
  if [[ "$priority" != "all" ]]; then
    endpoint=$(append_query_param "$endpoint" "priority" "$priority")
  fi
  if [[ -n "$category" ]]; then
    endpoint=$(append_query_param "$endpoint" "category" "$category")
  fi
  if [[ -n "$term" ]]; then
    endpoint=$(append_query_param "$endpoint" "search" "$term")
  fi
  if [[ "$ordering" == "newest" ]]; then
    endpoint=$(append_query_param "$endpoint" "ordering" "-created_at")
  fi

  browse_tickets "$endpoint" "Search Results"
}

ticket_details() {
  local ticket_id="$1"
  local details_file

  if ! api_request GET "/tickets/$ticket_id/"; then
    return 1
  fi

  details_file=$(mktemp) || {
    show_message "Could not create a temporary details file." "Client Error"
    return 1
  }

  jq -r '
    "ID: \(.id)",
    "Title: \(.title)",
    "Status: \(.status)",
    "Priority: \(.priority)",
    "Category: \(.category)",
    "Reporter: \(.reporter.username // "Unknown")",
    "Assigned to: \(.assigned_to.username // "Unassigned")",
    "Created: \(.created_at)",
    "Updated: \(.updated_at)",
    "Resolved: \(.resolved_at // "Not resolved")",
    "",
    "Description:",
    (.description // "")
  ' <<< "$API_RESPONSE" > "$details_file"

  dialog --title "Ticket $ticket_id" --textbox "$details_file" 0 0
  rm -f "$details_file"
  return 0
}

edit_ticket() {
  local ticket_id="$1"
  local title description category status priority payload

  if ! api_request GET "/tickets/$ticket_id/"; then
    return
  fi

  title=$(jq -r '.title // ""' <<< "$API_RESPONSE")
  description=$(jq -r '.description // ""' <<< "$API_RESPONSE")
  category=$(jq -r '.category // ""' <<< "$API_RESPONSE")

  if ! prompt_input "Edit Ticket" "Title:" "$title"; then
    return
  fi
  title="$INPUT_VALUE"

  if ! prompt_input "Edit Ticket" "Description:" "$description"; then
    return
  fi
  description="$INPUT_VALUE"

  if ! dialog_menu "Edit Ticket" "Choose status:" \
    open "Open" \
    in_progress "In progress" \
    resolved "Resolved" \
    closed "Closed" \
    reopened "Reopened"; then
    return
  fi
  status="$MENU_CHOICE"

  if ! dialog_menu "Edit Ticket" "Choose priority:" \
    low "Low" \
    normal "Normal" \
    high "High" \
    critical "Critical"; then
    return
  fi
  priority="$MENU_CHOICE"

  if ! prompt_input "Edit Ticket" "Category:" "$category"; then
    return
  fi
  category="$INPUT_VALUE"

  payload=$(jq -n \
    --arg title "$title" \
    --arg description "$description" \
    --arg status "$status" \
    --arg priority "$priority" \
    --arg category "$category" \
    '{
      title: $title,
      description: $description,
      status: $status,
      priority: $priority,
      category: $category
    }')

  if api_request PATCH "/tickets/$ticket_id/" "$payload"; then
    show_message "Ticket updated successfully." "Bug Tracker"
  fi
}

delete_ticket() {
  local ticket_id="$1"

  if ! dialog --title "Confirm Deletion" \
    --yesno "Permanently delete ticket $ticket_id?" 8 55; then
    return
  fi

  if api_request DELETE "/tickets/$ticket_id/"; then
    show_message "Ticket deleted successfully." "Bug Tracker"
  fi
}

ticket_actions() {
  local ticket_id="$1"

  while true; do
    if ! ticket_details "$ticket_id"; then
      return
    fi

    if ! dialog_menu "Ticket $ticket_id" "Choose an action:" \
      edit "Edit ticket" \
      delete "Delete ticket" \
      back "Back to ticket list"; then
      return
    fi

    case "$MENU_CHOICE" in
      edit)
        edit_ticket "$ticket_id"
        ;;
      delete)
        delete_ticket "$ticket_id"
        if [[ "$HTTP_STATUS" =~ ^2[0-9][0-9]$ ]]; then
          return
        fi
        ;;
      back)
        return
        ;;
    esac
  done
}

browse_tickets() {
  local endpoint="$1"
  local title="$2"
  local ticket_id ticket_title
  local -a menu_items

  while true; do
    if ! api_request GET "$endpoint"; then
      return
    fi

    menu_items=()
    while IFS=$'\t' read -r ticket_id ticket_title; do
      [[ -z "$ticket_id" ]] && continue
      menu_items+=("$ticket_id" "$ticket_title")
    done < <(jq -r '
      .results // [] | .[] |
      [
        (.id | tostring),
        (.title // "(untitled)" | gsub("[\\r\\n\\t]"; " "))
      ] | @tsv
    ' <<< "$API_RESPONSE" 2>/dev/null)

    if (( ${#menu_items[@]} == 0 )); then
      show_message "No tickets found." "$title"
      return
    fi

    if ! dialog_menu "$title" "Select a ticket:" "${menu_items[@]}"; then
      return
    fi

    ticket_actions "$MENU_CHOICE"
  done
}

create_ticket() {
  local title description priority category payload

  if ! prompt_input "Create Ticket" "Title:"; then
    return
  fi
  title="$INPUT_VALUE"

  if ! prompt_input "Create Ticket" "Description:"; then
    return
  fi
  description="$INPUT_VALUE"

  if ! dialog_menu "Create Ticket" "Choose priority:" \
    low "Low" \
    normal "Normal" \
    high "High" \
    critical "Critical"; then
    return
  fi
  priority="$MENU_CHOICE"

  if ! prompt_input "Create Ticket" "Category:"; then
    return
  fi
  category="$INPUT_VALUE"

  payload=$(jq -n \
    --arg title "$title" \
    --arg description "$description" \
    --arg priority "$priority" \
    --arg category "$category" \
    '{
      title: $title,
      description: $description,
      priority: $priority,
      category: $category
    }')

  if api_request POST "/tickets/" "$payload"; then
    show_message "Ticket created successfully." "Bug Tracker"
  fi
}

view_account() {
  local account

  if ! api_request GET "/auth/me/"; then
    return
  fi

  account=$(jq '.' <<< "$API_RESPONSE" 2>/dev/null)
  if [[ -z "$account" ]]; then
    account="$API_RESPONSE"
  fi

  show_message "$account" "Current Account"
}

logged_out_menu() {
  while [[ -z "$KEY" ]]; do
    if ! dialog_menu "Bug Tracker" "Choose an option:" \
      login "Log in" \
      register "Create account" \
      quit "Quit"; then
      continue
    fi

    case "$MENU_CHOICE" in
      login)
        login
        ;;
      register)
        register_account
        ;;
      quit)
        clear
        exit 0
        ;;
    esac
  done
}

authenticated_menu() {
  while [[ -n "$KEY" ]]; do
    if ! dialog_menu "Bug Tracker" "Choose an option:" \
      list "List tickets" \
      search "Search tickets" \
      create "Create ticket" \
      account "View account" \
      logout "Log out" \
      quit "Quit"; then
      continue
    fi

    case "$MENU_CHOICE" in
      list)
        browse_tickets "/tickets/" "Tickets"
        ;;
      search)
        search_tickets
        ;;
      create)
        create_ticket
        ;;
      account)
        view_account
        ;;
      logout)
        KEY=""
        ;;
      quit)
        clear
        exit 0
        ;;
    esac
  done
}

while true; do
  if [[ -z "$KEY" ]]; then
    logged_out_menu
  else
    authenticated_menu
  fi
done
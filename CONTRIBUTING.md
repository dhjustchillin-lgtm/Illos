# Contributing to Adamas Edition

First off, thanks for helping improve `Adamas Edition`! ❤️

All contributions are encouraged and valued. Please make sure to read the relevant section before making your contribution! It will make it a lot easier for you and the maintainers. We're excited to see your contributions. 🎉

## Bug Reports

We use [a custom system](./bug_tracker.sh) to track bugs.

### What should I do before making a bug report?

- Has somebody else already found this issue? This is best done by searching the [bug tracker](./bug_tracker.sh) to see if anybody else reported it. If there is already an issue, replying to the exsting issue with more information can help solve the problem.

### How do I submit a bug report?

If you run into an issue with the project, run the [bug tracker](./bug_tracker.sh).

The best bug reports have enough information that we won't have to contact you for more information. We welcome all efforts to improve Illos, but would be very grateful if you completed as much of the checklist as possible in your bug report. This will help other contributiors fix your issue.

### What happens after I submit a bug report?

- A maintainer will try to reproduce the bug with your provided steps.
    - If there are no reproduction steps or no obvious way to reproduce the issue, somebody will ask you for those steps. Until the bug can be reproduced, the bug will most likely not get fixed.
- If the team is able to reproduce the bug, it will be labeled as `in progress`, and the bug will be left to be fixed by someone.
    - If the issue is particularly game-breaking, a maintainer will add it to a future version's [milestone](), meaning that version will not be released until the problem is solved.

## Feature Requests

This section guides you through submitting a feature request for Illos, **including completely new features and minor improvements to existing functionality**. Following these guidelines will help maintainers and the community to understand your suggestion and find related suggestions.

- We use [a custom system](./bug_tracker.sh) issues to track feature requests.

### What should I do before making a feature request?

- Make sure your request is reasonable and fits the game - if it is not clear if something is in scope, you can start put it in anyways.

### How do I submit a feature request?

To request a feature to be added to the project, make a new ticket, with the category `"Feature request"`.

### What happens after I submit a feature request?

- A maintainer will check it out.
- If the feature request is out of the scope, it will be closed.
- if the request is in scope, any other contributor can volunteer to [fufill it via a pull request](#Pull-Requests). When the request is filled, the request will be closed.

## Pull Requests

### What should I do before starting a pull request?

- If you're new to git and GitHub, [Team Aqua's Asset Repo](https://github.com/Pawkkie/Team-Aquas-Asset-Repo/) has a [guide on forking and cloning the repository](https://github.com/Pawkkie/Team-Aquas-Asset-Repo/wiki/The-Basics-of-GitHub). Make sure you have a [local copy](INSTALL.md) of `pokeemerald-expansion`.
- Make sure your contribution is in [scope](docs/team_procedures/scope.md) - if it is not clear if something is in scope, you can start a discussion thread in the [#pr-discussions](https://discord.com/channels/419213663107416084/1102784418369785948) channel of the [the RHH Discord Server!](https://discord.gg/6CzjAG6GZk).
- Choose a branch to contribute your PR to:
    - **`master`**: If your PR contains meets any of the following criteria, it should go to `master`:
      - Fixes for bugs that are currently present in the `master` branch
      - PR is only documentation updates / changes
      - PR is only tests
    - **`upcoming`**: All other pull requests go to `upcoming`.
- Create a new branch from the most recent version of the branch you've chosen.
- If your contribution introduces, removes, or changes a lot of existing code, we reccomend getting a maintainer to agree to review it before you start on the work! We have a table that lists all [current maintainers and their areas of expertise](#maintainers).

### How do I submit a pull request?

#### 1. Get a working local copy
If you haven't already, follow [INSTALL.md](INSTALL.md) to get a working local copy of `pokeemerald-expansion`.

#### 2. Set RHH as a remote
This will designate the main `pokeemerald-expansion` repository as a remote.
```bash
git remote add RHH https://github.com/rh-hideout/pokeemerald-expansion # You can replace RHH with anything you want. This tutorial assumes you used RHH.
```

#### 3. Create a new branch
This will create a new branch and switch to it.
```bash
git switch -c newFeature # the name newFeature can be anything you want. This tutorial assumes you used newFeature.
```

#### 4. Copy your target branch to your new branch
This will change your new branch to match the latest version of your chosen target branch.
```bash
git reset --hard upcoming # If your PR is going to target master, replace upcoming with master.
```

#### 5. Implement your code
All of your work should go on this new, clean branch. If you already started work on a different branch, you can [cherry-pick](https://git-scm.com/docs/git-cherry-pick) your old commits onto this new branch, or just copy and paste the changes from the original files.

##### Popular Features / Feature Branches

If you are implementing functionality from a known community feature branch, it is **strongly** recommended that you open a discussion thread _before_ starting. There are some situations where maintainers would ask you to use the existing feature branch as a base, and others where maintainers would want a feature to be written from scratch.

This changes on a case by case basis.

#### 6. Push your changes
When you push your first commit, you'll need to push the new branch to the remote repo.
```bash
git push --set-upstream origin newFeature
```

#### 7. Open Pull Request
Once your work is complete and pushed to the branch on Github, you can open a [pull request from your branch](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request-from-a-fork), targeting the branch you've chosen from `pokeemerald-expansion`. Please fill out the pull request description as completely as possible.

### What happens after I submit a pull request?

A maintainer will then assign themselves as a reviewer of your pull request, and may provide feedback in the form of a PR review.

Contributors are responsible for responding to and updating their branch by addressing the feedback in the review. Contributors are also responsible for making sure the branch passes the checklist at all times.

Once a maintainer has begun reviewing your PR, **please** do not force-push new changes - normal pushes are fine. Do not worry about git history - we squash most incoming changes.

Maintainers will measure the submitted pull request against a [merge checklist](docs/team_procedures/merge_checklist.md).

Once all items on the merge checklist are true, the branch will be merged in.


## Maintainers

This list was last updated 2026 October 19.

| Name | Currently Active | Areas of Expertise |
| --- | --- | --- |
| [Dworks](https://github.com/dhjustchillin-lgtm) | ✅ | The game.

## Attribution
This guide is based on the [contributing.md](https://contributing.md/generator)!

# Testing Pull Requests (PRs)

When a PR is submitted, it is compiled automatically by AppVeyor and Travis CI. The AppVeyor build creates an artifact which contains the Tahoma2D executable and supporting DLLs needed to run.  You can easily download this artifact and run it to test out the PR before it is merged.

## Pre-Testing Notes
- This can only be tested on Windows. There is no OSX or Linux build that can be tested.
- It is recommended when testing to use new scenes that you can throw away.
  - If you decide to test on an existing scene, back it up first!

## Downloading and Testing AppVeyor Builds

- Go to the PR you are interested in checking out.

- Scroll to the bottom of the page and look for the green-outlined box with `All checks have passed` in it. Click on `Show all checks` next to it.

![](./testing_pr_1.JPG)

- Look for where it says `AppVeyor build succeeded` and click on the `Details` next to it.

![](./testing_pr_2.JPG)

- Click on `Configuration: Release`.

![](./testing_pr_3.JPG)

- On the right side of the page, click on `Artifacts`.

![](./testing_pr_4.JPG)

- Click on the Tahoma2D `.zip` file to begin the download.

![](./testing_pr_5.JPG)

- Once download is complete, go to your Downloads folder and extract the contents of the .zip file anywhere on your computer.
 - You should extract into a separate folder. 
 - ⚠️ **Do not overwrite your current Tahoma2D installation!**

- Enter the folder you extracted everything into and double-click the Tahoma2D executable to start.

- Test away!  Report any suggestions or issues related to the change in the PR comments.

- You can safely delete the .zip file and the folder you created when you are done.

# License Notes <!-- omit in toc -->

> :warning: This document is a work in progress

- [Terminology](#terminology)
- [Credits](#credits)
- [Original License](#original-license)
- [New License](#new-license)
- [GPL / MIT](#gpl--mit)
- [Why did we want to change the license?](#why-did-we-want-to-change-the-license)
- [How did we change the license?](#how-did-we-change-the-license)
- [Can we provide more details on the re-licensing](#can-we-provide-more-details-on-the-re-licensing)
- [Why didn't we do a copyright transfer?](#why-didnt-we-do-a-copyright-transfer)
- [Long term goal](#long-term-goal)

## Terminology

'We' / 'Us' - This document represents the views of the ['dasher-project'](https://github.com/dasher-project) [(stewarded by Ace Centre)](https://github.com/dasher-project/governance), importantly not the Inference group.

'Stewarded' - It is important to note that Ace Centre asserts no ownership over Dasher. Ace Centre aims to facilitate discussion and development not to 'own' Dasher or make major decisions in isolation. [You can see the governance structure here.](https://github.com/dasher-project/governance)

## Credits

First of all its important to give important credit to the people who originally developed Dasher. Dasher was developed by the Inference Group in Cavendish Laboratory, Cambridge.

> David MacKay's group works on machine learning and information theory. Current projects involve neural networks, automated Go playing, the design of record-breaking error-correcting codes and quantum error-correcting codes, and the construction of human-computer interfaces that make use of adaptive language models.

Dasher is a ground breaking development in text entry using continuous gestures and to this day it is still seen as the most effective way to enter text using continuous gestures. All of this is thanks to David MacKay and his hard working team.

[You can see an archive of the groups website here.](http://www.inference.org.uk/is/). The inference group are in the process of setting up a consultation body which will contribute to the further development of Dasher.

## Original License

Dasher was originally developed and published with a GPL license.

[You can still use the fully GPL version here.](https://gitlab.gnome.org/GNOME/dasher). This version has been carefully maintained by Patrick Welche and others since its creation. The GPL version is still under development.

## New License

[This version of Dasher](https://github.com/dasher-project/dasher-mit) is licensed under the MIT License.

## GPL / MIT

GPL is a copy left license, meaning:

> Permissions of this strong copyleft license are conditioned on making available complete source code of licensed works and modifications, which include larger works using a licensed work, under the same license. - [Choose a license](https://choosealicense.com/licenses/gpl-3.0/)

MIT is permissive, meaning:

> A short and simple permissive license with conditions only requiring preservation of copyright and license notices. Licensed works, modifications, and larger works may be distributed under different terms and without source code. - [Choose a license](https://choosealicense.com/licenses/mit/)

## Why did we want to change the license?

To understand why we wanted to change the license it is helpful to understand our main goals. In short, we want more people to be able to access Dasher and we would like to see active development of Dasher.

To achieve those goals we identified it would be best to change the license to MIT

## How did we change the license?

To change the license we embarked first seeking legal advice from [FSFE](https://fsfe.org/index.en.html). To change the license of a project everyone who has contributed to a project must agree to have their contribution available under the new license. In the case where the contributor has passed away we must ask the person their copyright was passed to for the permission.

To identify copyright holders we used the Git history of the project, specifically looking at the git 'author' field.

After doing this we discovered that there was over 200 contributors to the project. We felt that it wouldn't be feasible to contact all of these people to ask for permission to re-license the project. To only re-license the project we only need permission from people who have contributions that still exists in the project.

## Can we provide more details on the re-licensing

I am happy to answer any questions you may have. You can reach me on the Slack group (@gavinhenderson) or you can email me at ghenderson@acecentre.org.uk

The documents containing the permission to relicense Dasher from each contributor are available upon request. They are only kept private to protect the contact details of individuals private.

## Why didn't we do a copyright transfer?

Whilst we were gaining permission we could have asked copyright holders to transfer their copyright to a central entity allowing us to change the license in the future. However, we chose not to do this as we feel this fundamentally goes against the idea of open source. For the same reason we don't ask contributors to transfer copyright.

## Long term goal

Right now there are two versions of Dasher in development and maintenance. One is this version which is MIT licensed and the other is GPL licensed. Neither is the 'main' version, they are both taking different approaches and are both under development. Both teams work together together to try and get more people using Dasher effectively.

We would like to bring these to codebases together somehow. We are not quite sure what that looks like but that is the long term goal.

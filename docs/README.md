#  Guide for working on reflect-cpp docs

## Setup a dev environment

The following steps should be followed to setup an environment for working on the documentation for the reflect-cpp:

### 1. Install Hatch

We use [Hatch](https://hatch.pypa.io/latest/) to manage the project dependencies. Depending on your system, follow the instructions on the following webpage to install it: [https://hatch.pypa.io/latest/install/](https://hatch.pypa.io/latest/install/).

### 2. Clone reflect-cpp repo.

If you haven’t already, clone the GitHub repository of reflect-cpp as follows:

```shell
git clone https://github.com/getml/reflect-cpp.git
```

### 3. Set up hatch environment

1. Change the working directory to inside the repository and switch to *main* branch:
    
    ```shell
    cd reflect-cpp
    git switch main
    ```

2. Enter a shell within the *default* environment of the project:

    ```shell
    hatch shell
    ```

    This will install the required dependencies mentioned in `pyproject.toml` in the environment.

### 4. Serve the documentation

Once inside the shell of the *default* environment, you can execute the following to serve the docs on the builtin development server of mkdocs:
    
```bash
mkdocs serve
```    

This is it. You have setup the environment to work with the reflect-cpp docs.

Note: The *insiders* environment specified in `pyproject.toml` uses the insiders edition of material theme. This environment is used during the GitHub workflow for building and deploying the documentation. The features of insiders edition are made [compatible](https://squidfunk.github.io/mkdocs-material/insiders/access-management/#outside-collaborators) with the community edition with the use of `group` plugin.
pipeline {
    options {
        disableConcurrentBuilds()
        timeout(time: 8, unit: "HOURS")
        timestamps()
    }
    agent {
        node { label "master" }
    }
    parameters {
        choice(name: "CHANNEL", choices: ["dev", "beta", "release", "nightly"])
        booleanParam(name: "WIPE_WORKSPACE", defaultValue: false)
        booleanParam(name: "RUN_INIT", defaultValue: false)
        booleanParam(name: "DISABLE_SCCACHE", defaultValue: false)
    }
    environment {
        CHANNEL = "${params.CHANNEL}"
        WIPE_WORKSPACE = "${params.WIPE_WORKSPACE}"
        RUN_INIT = "${params.RUN_INIT}"
        DISABLE_SCCACHE = "${params.DISABLE_SCCACHE}"
        GITHUB_CREDENTIAL_ID = "brave-builds-github-token-for-pr-builder"
        GITHUB_AUTH = credentials("${GITHUB_CREDENTIAL_ID}")
        GITHUB_API = "https://api.github.com/repos/brave"
        BB_REPO = "https://${GITHUB_AUTH}@github.com/brave/brave-browser"
    }
    stages {
        stage("env") {
            steps {
                script {
                    env.BRANCH_TO_BUILD = (env.CHANGE_BRANCH == null ? env.BRANCH_NAME : env.CHANGE_BRANCH)
                    env.TARGET_BRANCH = (env.CHANGE_TARGET == null ? env.BRANCH_TO_BUILD : env.CHANGE_TARGET)
                    env.BRANCH_EXISTS_IN_BB = httpRequest(url: env.GITHUB_API + "/brave-browser/branches/" + env.BRANCH_TO_BUILD, validResponseCodes: "100:499", authentication: env.GITHUB_CREDENTIAL_ID).status == 200
                    response = httpRequest(url: env.GITHUB_API + "/brave-core/pulls?head=brave:" + env.BRANCH_TO_BUILD, authentication: env.GITHUB_CREDENTIAL_ID)
                    prDetails = readJSON(text: response.content)[0]
                    response = httpRequest(url: env.GITHUB_API + "/brave-core/pulls/" + prDetails.number, authentication: env.GITHUB_CREDENTIAL_ID)
                    env.PR_IS_DRAFT = readJSON(text: response.content).mergeable_state == "draft"
                }
            }
        }
        stage("check") {
            when {
                expression { env.PR_IS_DRAFT == "true" }
            }
            steps {
                script {
                    print "Aborting build as commit is in draft!"
                    currentBuild.result = "ABORTED"
                }
            }
        }
        stage("config") {
            when {
                expression { env.PR_IS_DRAFT == "false" }
            }
            steps {
                sh """
                    #!/bin/bash

                    set +x

                    if [ -d brave-browser ]; then
                        rm -rf brave-browser
                    fi

                    git clone --branch ${TARGET_BRANCH} ${BB_REPO} || git clone ${BB_REPO}

                    cd brave-browser

                    git config user.name brave-builds
                    git config user.email devops@brave.com
                    git config push.default simple

                    if [ "${BRANCH_EXISTS_IN_BB}" = "false" ]; then
                        git checkout -b ${BRANCH_TO_BUILD}

                        echo "Pinning brave-core to branch ${BRANCH_TO_BUILD}..."
                        jq "del(.config.projects[\\"brave-core\\"].branch) | .config.projects[\\"brave-core\\"].branch=\\"${BRANCH_TO_BUILD}\\"" package.json > package.json.new
                        mv package.json.new package.json

                        echo "Committing..."
                        git commit --all --message "pin brave-core to branch ${BRANCH_TO_BUILD}"

                        echo "Pushing changes..."
                        git push ${BB_REPO}
                    else
                        git checkout ${BRANCH_TO_BUILD}

                        if [ "`cat package.json | jq -r .version`" != "`cat ../package.json | jq -r .version`" ]; then
                            set +e

                            echo "Version mismatch between brave-browser and brave-core in package.json! Attempting rebase on brave-browser..."

                            echo "Fetching latest changes and pruning refs..."
                            git fetch --prune

                            echo "Rebasing ${BRANCH_TO_BUILD} branch on brave-browser against ${TARGET_BRANCH}..."
                            git rebase origin/${TARGET_BRANCH}

                            if [ \$? -ne 0 ]; then
                                echo "Failed to rebase (conflicts), will need to be manually rebased!"
                                git rebase --abort
                            else
                                echo "Rebased, force pushing to brave-browser..."
                                git push --force ${BB_REPO}
                            fi

                            if [ "`cat package.json | jq -r .version`" != "`cat ../package.json | jq -r .version`" ]; then
                                echo "Version mismatch between brave-browser and brave-core in package.json! Please try rebasing this branch in brave-core as well."
                                exit 1
                            fi

                            set -e
                        fi
                    fi

                    echo "Sleeping 6m so new branch is discovered or associated PR created in brave-browser..."
                    sleep 360
                """
            }
        }
        stage("build") {
            when {
                expression { env.PR_IS_DRAFT == "false" }
            }
            steps {
                script {
                    response = httpRequest(url: env.GITHUB_API + "/brave-browser/pulls?head=brave:" + env.BRANCH_TO_BUILD, authentication: env.GITHUB_CREDENTIAL_ID)
                    prDetails = readJSON(text: response.content)[0]
                    prNumber = prDetails ? prDetails.number : ""
                    refToBuild = prNumber ? "PR-" + prNumber : URLEncoder.encode(env.BRANCH_TO_BUILD, "UTF-8")
                    params = [
                        string(name: "CHANNEL", value: env.CHANNEL),
                        booleanParam(name: "WIPE_WORKSPACE", value: env.WIPE_WORKSPACE),
                        booleanParam(name: "RUN_INIT", value: env.RUN_INIT),
                        booleanParam(name: "DISABLE_SCCACHE", value: env.DISABLE_SCCACHE)
                    ]
                    currentBuild.result = build(job: "brave-browser-build-pr/" + refToBuild, parameters: params, propagate: false).result
                }
            }
        }
    }
}

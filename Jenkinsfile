pipeline {
    options {
        disableConcurrentBuilds()
        timeout(time: 1, unit: "DAYS")
        timestamps()
    }
    agent {
        node { label "master" }
    }
    environment {
        GITHUB_CREDENTIALS = credentials("brave-builds-github-token-for-pr-builder")
        BB_REPO = "https://${GITHUB_CREDENTIALS}@github.com/brave/brave-browser"
    }
    stages {
        stage("env") {
            steps {
                script {
                    env.BRANCH_TO_BUILD = (env.CHANGE_BRANCH == null ? env.BRANCH_NAME : env.CHANGE_BRANCH)
                    env.TARGET_BRANCH = (env.CHANGE_TARGET == null ? env.BRANCH_TO_BUILD : env.CHANGE_TARGET)
                }
            }
        }
        stage("config") {
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

                    if [ "`curl -s -w %{http_code} -o /dev/null ${BB_REPO}/tree/${BRANCH_TO_BUILD}`" = "404" ]; then
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

                    echo "Sleeping 5m so new branch is discovered or associated PR created in brave-browser..."
                    sleep 300
                """
            }
        }
        stage("build") {
            steps {
                script {
                    response = httpRequest(url: "https://api.github.com/repos/brave/brave-browser/pulls?head=brave:${BRANCH_TO_BUILD}", quiet: true)
                    prDetails = readJSON(text: response.content)[0]
                    prNumber = prDetails ? prDetails.number : ""
                    refToBuild = prNumber ? "PR-" + prNumber : URLEncoder.encode("${BRANCH_TO_BUILD}", "UTF-8")
                    currentBuild.result = build(job: "brave-browser-build-pr/" + refToBuild, propagate: false).result
                }
            }
        }
    }
}

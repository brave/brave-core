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
                    if [ -d brave-browser ]; then
                        rm -rf brave-browser
                    fi

                    echo "Cloning brave-browser..."
                    git clone -b ${TARGET_BRANCH} ${BB_REPO} || git clone ${BB_REPO}
                    cd brave-browser

                    git config user.name brave-builds
                    git config user.email devops@brave.com
                    git config push.default simple

                    if [ "`curl -s -w %{http_code} -o /dev/null ${BB_REPO}/tree/${BRANCH_TO_BUILD}`" = "404" ]; then
                        echo "Creating ${BRANCH_TO_BUILD} branch in brave-browser..."
                        git checkout -f -b ${BRANCH_TO_BUILD}

                        echo "Pinning brave-core to branch ${BRANCH_TO_BUILD}..."
                        jq "del(.config.projects[\\"brave-core\\"].branch) | .config.projects[\\"brave-core\\"].branch=\\"${BRANCH_TO_BUILD}\\"" package.json > package.json.new
                        mv package.json.new package.json

                        echo "Pushing changes..."
                        git commit -a -m "pin brave-core to branch ${BRANCH_TO_BUILD}"
                        git push ${BB_REPO}
                    else
                        echo "Rebasing brave-browser branch against ${TARGET_BRANCH}..."
                        git checkout ${BRANCH_TO_BUILD}

                        set +e
                        git fetch -p
                        git rebase origin/${TARGET_BRANCH}
                        if [ \$? -ne 0 ]; then
                            echo "Failed to rebase (conflicts), will need to be manually rebased!"
                            git rebase --abort
                        else
                            echo "Rebased ${BRANCH_TO_BUILD} against ${TARGET_BRANCH}"
                            git push ${BB_REPO}
                        fi
                        set -e
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

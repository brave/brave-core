pipeline {
    options {
        disableConcurrentBuilds()
        timeout(time: 4, unit: "HOURS")
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
                    if [ "`curl -s -w %{http_code} -o /dev/null ${BB_REPO}/tree/${BRANCH_TO_BUILD}`" = "404" ]; then
                        if [ -d brave-browser ]; then
                            rm -rf brave-browser
                        fi

                        echo "Cloning brave-browser..."
                        git clone -b ${TARGET_BRANCH} ${BB_REPO} || git clone ${BB_REPO}
                        cd brave-browser

                        git config user.name brave-builds
                        git config user.email devops@brave.com
                        git config push.default simple

                        echo "Creating ${BRANCH_TO_BUILD} branch in brave-browser..."
                        git checkout -f -b ${BRANCH_TO_BUILD}

                        echo "Pinning brave-core to branch ${BRANCH_TO_BUILD}..."
                        jq "del(.config.projects[\\"brave-core\\"].branch) | .config.projects[\\"brave-core\\"].branch=\\"${BRANCH_TO_BUILD}\\"" package.json > package.json.new
                        mv package.json.new package.json

                        echo "Pushing changes..."
                        git commit -a -m "pin brave-core to branch ${BRANCH_TO_BUILD}"
                        git push ${BB_REPO}
                    fi

                    echo "Sleeping 5m so new branch is discovered or associated PR created in brave-browser..."
                    sleep 300
                """
            }
        }
        stage("build") {
            steps {
                script {
                    def response = httpRequest(url: "https://api.github.com/repos/brave/brave-browser/pulls?head=brave:${BRANCH_TO_BUILD}")
                    def prDetails = readJSON(text: response.content)[0]
                    def prNumber = prDetails ? prDetails.number : ""
                    def refToBuild = prNumber ? "PR-" + prNumber : "${BRANCH_TO_BUILD}"
                    def buildResult = build(job: "brave-browser-build-pr/" + refToBuild, propagate: false).result

                    echo "Browser build ${buildResult} at ${JENKINS_URL}/job/brave-browser-build-pr/" + refToBuild

                    if (buildResult == "ABORTED") {
                        currentBuild.result = "FAILURE"
                    }
                    else {
                        currentBuild.result = buildResult
                    }
                }
            }
        }
    }
}

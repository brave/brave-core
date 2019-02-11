pipeline {
    options {
        disableConcurrentBuilds()
        timeout(time: 4, unit: "HOURS")
        timestamps()
    }
    agent {
        node { label 'master' }
    }
    environment {
        GITHUB_CREDENTIALS = credentials('brave-builds-github-token-for-pr-builder')
        BC_REPO = "https://${GITHUB_CREDENTIALS}@github.com/brave/brave-browser"
    }
    stages {
        stage('config') {
            steps {
                sh """
                    if [ -d brave-browser ]; then
                        rm -rf brave-browser
                    fi

                    echo "Cloning brave-browser..."
                    git clone -b ${CHANGE_TARGET} ${BC_REPO}

                    cd brave-browser

                    if [ -z "`git ls-remote --heads ${BC_REPO} ${CHANGE_BRANCH}`" ]; then
                        git config user.name brave-builds
                        git config user.email devops@brave.com
                        git config push.default simple

                        echo "Creating ${CHANGE_BRANCH} branch in brave-browser..."
                        git checkout -f -b ${CHANGE_BRANCH}

                        echo "Pinning brave-core to branch ${CHANGE_BRANCH}..."
                        jq "del(.config.projects[\\"brave-core\\"].branch) | .config.projects[\\"brave-core\\"].branch=\\"${CHANGE_BRANCH}\\"" package.json > package.json.new
                        mv package.json.new package.json

                        echo "Pushing changes..."
                        git commit -a -m "pin brave-core to branch ${CHANGE_BRANCH}"
                        git push ${BC_REPO}

                        echo "Sleeping 5m so new branch is discovered..."
                        sleep 300
                    fi
                """
            }
        }
        stage('build') {
            steps {
                script {
                    def buildResult = build(job: "brave-browser-build-pr/${CHANGE_BRANCH}", propagate: false).result
                    echo "Browser build ${buildResult} at ${JENKINS_URL}/job/brave-browser-build-pr/${CHANGE_BRANCH}"
                    if (buildResult == 'ABORTED') { currentBuild.result = 'FAILURE' } else { currentBuild.result = buildResult }
                }
            }
        }
    }
}

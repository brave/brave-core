pipeline {
    options {
        disableConcurrentBuilds()
        timestamps()
    }
    agent {
        node { label 'master' }
    }
    environment {
        GITHUB_CREDENTIALS = credentials('brave-builds-github-token-for-pr-builder')
        BC_REPO = "https://${GITHUB_CREDENTIALS}@github.com/brave/brave-browser"
        BC_BRANCH = "brave-core-${GIT_BRANCH}"
    }
    stages {
        stage('checkout') {
            steps {
                sh """
                    if [ -d brave-browser ]; then
                        rm -rf brave-browser
                    fi

                    git clone -b ${CHANGE_TARGET} ${BC_REPO}

                    cd brave-browser

                    if [ -z "`git ls-remote --heads ${BC_REPO} ${BC_BRANCH}`" ]; then
                        git checkout -f -b ${BC_BRANCH}
                    else
                        git checkout -f ${BC_BRANCH}
                    fi
                """
            }
        }
        stage('config') {
            steps {
                sh """
                    git config -f brave-browser/.git/config user.name brave-builds
                    git config -f brave-browser/.git/config user.email devops@brave.com
                    git config -f brave-browser/.git/config push.default simple
                """
            }
        }
        stage('pin') {
            steps {
                sh """
                    jq "del(.config.projects[\\"brave-core\\"].branch) | .config.projects[\\"brave-core\\"].commit=\\"${GIT_COMMIT}\\"" brave-browser/package.json > brave-browser/package.json.new
                    mv brave-browser/package.json.new brave-browser/package.json
                """
            }
        }
        stage('push') {
            steps {
                sh """
                    git -C brave-browser commit -a -m "pin brave-core to ${GIT_COMMIT} from ${GIT_BRANCH}"
                    git -C brave-browser push ${BC_REPO}
                """
            }
        }
        stage('sleep') {
            steps {
                sleep time: 5, unit: 'MINUTES'
            }
        }        
        stage('build') {
            steps {
                script {
                    def buildResult = build(job: "brave-browser-build-pr/${BC_BRANCH}", propagate: false).result
                    echo "Building browser result is ${buildResult}"
                    if (buildResult == 'ABORTED') { currentBuild.result = 'FAILURE' } else { currentBuild.result = buildResult }
                }
            }
        }
        stage('clean') {
            steps {
                sh "git -C brave-browser push origin :${BC_BRANCH}"
            }
        }
    }
}

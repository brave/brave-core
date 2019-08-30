pipeline {
    agent {
        node { label "master" }
    }
    options {
        timeout(time: 6, unit: "HOURS")
        timestamps()
    }
    parameters {
        choice(name: "BUILD_TYPE", choices: ["Release", "Debug"], description: "")
        choice(name: "CHANNEL", choices: ["development", "nightly", "dev", "beta", "release"], description: "")
        booleanParam(name: "OFFICIAL_BUILD", defaultValue: true, description: "")
        booleanParam(name: "SKIP_SIGNING", defaultValue: false, description: "")
        booleanParam(name: "WIPE_WORKSPACE", defaultValue: false, description: "")
        booleanParam(name: "SKIP_INIT", defaultValue: false, description: "")
        booleanParam(name: "DISABLE_SCCACHE", defaultValue: false, description: "")
        booleanParam(name: "DEBUG", defaultValue: false, description: "")
    }
    environment {
        GITHUB_API = "https://api.github.com/repos/brave"
        GITHUB_CREDENTIAL_ID = "brave-builds-github-token-for-pr-builder"
        GITHUB_AUTH = credentials("${GITHUB_CREDENTIAL_ID}")
        BB_REPO = "https://${GITHUB_AUTH}@github.com/brave/brave-browser"
    }
    stages {
        stage("env") {
            steps {
                script {
                    BUILD_TYPE = params.BUILD_TYPE
                    CHANNEL = params.CHANNEL
                    OFFICIAL_BUILD = params.OFFICIAL_BUILD
                    SKIP_SIGNING = params.SKIP_SIGNING
                    WIPE_WORKSPACE = params.WIPE_WORKSPACE
                    SKIP_INIT = params.SKIP_INIT
                    DISABLE_SCCACHE = params.DISABLE_SCCACHE
                    DEBUG = params.DEBUG
                    SKIP = false
                    BRANCH = env.BRANCH_NAME
                    TARGET_BRANCH = "master"
                    if (env.CHANGE_BRANCH) {
                        BRANCH = env.CHANGE_BRANCH
                        TARGET_BRANCH = env.CHANGE_TARGET
                        def prNumber = readJSON(text: httpRequest(url: GITHUB_API + "/brave-core/pulls?head=brave:" + BRANCH, authentication: GITHUB_CREDENTIAL_ID, quiet: !DEBUG).content)[0].number
                        def prDetails = readJSON(text: httpRequest(url: GITHUB_API + "/brave-core/pulls/" + prNumber, authentication: GITHUB_CREDENTIAL_ID, quiet: !DEBUG).content)
                        SKIP = prDetails.mergeable_state.equals("draft") or prDetails.labels.count { label -> label.name.equalsIgnoreCase("CI/skip") }.equals(1)
                    }
                    BRANCH_EXISTS_IN_BB = httpRequest(url: GITHUB_API + "/brave-browser/branches/" + BRANCH, validResponseCodes: "100:499", authentication: GITHUB_CREDENTIAL_ID, quiet: !DEBUG).status.equals(200)
                }
            }
        }
        stage("abort") {
            steps {
                script {
                    if (SKIP) {
                        echo "Aborting build as PR is in draft or has \"CI/skip\" label"
                        stopCurrentBuild()
                    }
                    for (build in getBuilds()) {
                        if (build.isBuilding() && build.getNumber() < env.BUILD_NUMBER.toInteger()) {
                            echo "Aborting older running build " + build
                            build.doStop()
                            // build.finish(hudson.model.Result.ABORTED, new java.io.IOException("Aborting build"))
                        }
                    }
                }
            }
        }
        stage("checkout") {
            when {
                expression { !SKIP }
            }
            steps {
                sh """
                    set -e

                    if [ -d brave-browser ]; then
                        rm -rf brave-browser
                    fi

                    git clone --branch ${TARGET_BRANCH} ${BB_REPO} || git clone ${BB_REPO}

                    cd brave-browser
                    git config user.name brave-builds
                    git config user.email devops@brave.com
                    git config push.default simple
                """
            }
        }
        stage("branch-create") {
            when {
                allOf {
                    expression { !SKIP }
                    expression { !BRANCH_EXISTS_IN_BB }
                }
            }
            steps {
                sh """
                    set -e

                    cd brave-browser
                    git checkout -b ${BRANCH}

                    echo "Creating CI branch"
                    git commit --allow-empty -m "created CI branch"

                    echo "Pushing"
                    git push ${BB_REPO}
                """
            }
        }
        stage("branch-rebase") {
            when {
                allOf {
                    expression { !SKIP }
                    expression { BRANCH_EXISTS_IN_BB }
                }
            }
            steps {
                sh """
                    set -e

                    cd brave-browser
                    git checkout ${BRANCH}

                    if [ "`jq -r .version package.json`" != "`jq -r .version ../package.json`" ]; then
                        set +e

                        echo "Version mismatch between brave-browser and brave-core in package.json, attempting rebase on brave-browser"

                        echo "Fetching latest changes and pruning refs"
                        git fetch --prune

                        echo "Rebasing ${BRANCH} branch on brave-browser against ${TARGET_BRANCH}"
                        git rebase origin/${TARGET_BRANCH}

                        if [ \$? -ne 0 ]; then
                            echo "Failed to rebase (conflicts), will need to be manually rebased"
                            git rebase --abort
                        else
                            echo "Rebased, force pushing to brave-browser"
                            git push --force ${BB_REPO}
                        fi

                        if [ "`jq -r .version package.json`" != "`jq -r .version ../package.json`" ]; then
                            echo "Version mismatch between brave-browser and brave-core in package.json, please try rebasing this branch in brave-core as well"
                            exit 1
                        fi

                        set -e
                    fi
                """
            }
        }
        stage("build") {
            when {
                expression { !SKIP }
            }
            steps {
                script {
                    try {
                        startBraveBrowserBuild()
                    }
                    catch (hudson.AbortException ex) {
                        echo "Sleeping 6m so new branch is discovered or associated PR created in brave-browser"
                        sleep(time: 6, unit: "MINUTES")
                        startBraveBrowserBuild()
                    }
                }
            }
        }
    }
}

@NonCPS
def stopCurrentBuild() {
    Jenkins.instance.getItemByFullName(env.JOB_NAME).getLastBuild().doStop()
}

@NonCPS
def getBuilds() {
    return Jenkins.instance.getItemByFullName(env.JOB_NAME).builds
}

def startBraveBrowserBuild() {
    def prDetails = readJSON(text: httpRequest(url: GITHUB_API + "/brave-browser/pulls?head=brave:" + BRANCH, authentication: GITHUB_CREDENTIAL_ID, quiet: !DEBUG).content)[0]
    def prNumber = prDetails ? prDetails.number : ""
    def refToBuild = prNumber ? "PR-" + prNumber : URLEncoder.encode(BRANCH, "UTF-8")
    params = [
        string(name: "BUILD_TYPE", value: BUILD_TYPE),
        string(name: "CHANNEL", value: CHANNEL),
        booleanParam(name: "OFFICIAL_BUILD", value: OFFICIAL_BUILD),
        booleanParam(name: "SKIP_SIGNING", value: SKIP_SIGNING),
        booleanParam(name: "WIPE_WORKSPACE", value: WIPE_WORKSPACE),
        booleanParam(name: "SKIP_INIT", value: SKIP_INIT),
        booleanParam(name: "DISABLE_SCCACHE", value: DISABLE_SCCACHE),
        booleanParam(name: "DEBUG", value: DEBUG)
    ]
    currentBuild.result = build(job: "brave-browser-build-pr/" + refToBuild, parameters: params, propagate: false).result
}

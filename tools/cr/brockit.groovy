pipeline {
    agent any
    options {
        ansiColor('xterm-256color')
    }
    parameters {
        string(name: 'BRANCH', defaultValue: 'master', description: 'The base to be used when creating a branch for these changes')
        string(name: 'CHROMIUM_VERSION', defaultValue: '136.0.7103.4', description: 'The full Chromium version; ex: 135.0.7049.52')
    }
    environment {
        CHROMIUM_SRC_PATH = "brave-browser/src/"
        BRAVE_CORE_PATH = "brave-browser/src/brave/"
        PATH = "${WORKSPACE}/${BRAVE_CORE_PATH}/vendor/depot_tools:${PATH}"
    }
    stages {
        stage('setup') {
            steps {
                script {
                    // Define failureReason as a global variable
                    failureReason = null
                }
            }
        }
        stage('checkout') {
            steps {
                script {
                    if (!params.CHROMIUM_VERSION.matches(/^\d+\.\d+\.\d+\.\d+$/)) {
                        error("Invalid CHROMIUM_VERSION format. Expected format: MAJOR.MINOR.BUILD.PATCH (e.g., 135.0.7049.52)")
                    }
                    def majorVersion = params.CHROMIUM_VERSION.split('\\.')[0]
                    branchName = params.BRANCH == 'master' ? "cr${majorVersion}.x" : "cr${majorVersion}.x-on-${params.BRANCH}"
                    sh """
                    cd ${BRAVE_CORE_PATH}
                    git fetch origin ${params.BRANCH}
                    git branch -D ${branchName} || true
                    git checkout -b ${branchName} origin/${params.BRANCH}
                    git reset --hard origin/canary
                    """
                }
            }
        }
        stage('init') {
            steps {
                dir("${BRAVE_CORE_PATH}") {
                    sh """
                    npm run init -- --with_issue_44921
                    """
                }
            }
        }
        stage('lift') {
            steps {
                dir("${BRAVE_CORE_PATH}") {
                    script {
                        def exitCode = sh(
                            script: "vpython3 tools/cr/brockit.py lift --to=${params.CHROMIUM_VERSION} --verbose --infra-mode",
                            returnStatus: true
                        )
                        echo "Lift process exited with code: ${exitCode}"
                        if (exitCode != 0) {
                            echo "Lift process failed. Determining reason for failure..."
                            failureReason = sh(
                                script: "vpython3 tools/cr/brockit.py show --reason-for-lift-failure-for=${params.CHROMIUM_VERSION}",
                                returnStdout: true
                            ).trim()
                            if (failureReason != 'Advisories' && failureReason != 'Continuance') {
                                error("Lift process failed with exit code ${exitCode} and an unrecognized reason: ${failureReason}")
                            }
                        } else {
                            failureReason = null // Reset failureReason on success
                        }
                    }
                }
            }
        }
        stage('advisories') {
            when {
                expression { failureReason == 'Advisories' }
            }
            steps {
                dir("${BRAVE_CORE_PATH}") {
                    script {
                        input message: "Advisory alerts have been shown. Please review the 🚀 output. Fixes can be submitted to origin/${branchName} before continuing.",
                              ok: "I acknowledge the advisories"

                        def ackExitCode = sh(
                            script: "vpython3 tools/cr/brockit.py lift --to=${params.CHROMIUM_VERSION} --verbose --infra-mode --ack-advisory",
                            returnStatus: true
                        )
                        echo "Acknowledgment process exited with code: ${ackExitCode}"
                        if (ackExitCode != 0) {
                            echo "Acknowledgment process failed. Determining reason for failure..."
                            failureReason = sh(
                                script: "vpython3 tools/cr/brockit.py show --reason-for-lift-failure-for=${params.CHROMIUM_VERSION}",
                                returnStdout: true
                            ).trim()
                            if (!failureReason) {
                                error("Acknowledgment process failed with exit code ${ackExitCode} and no failure reason provided.")
                            }
                            echo "Updated failure reason: ${failureReason}"
                        }
                    }
                }
            }
        }
        stage('conflict') {
            when {
                expression { failureReason == 'Continuance' }
            }
            steps {
                dir("${BRAVE_CORE_PATH}") {
                    script {
                        while (failureReason == 'Continuance') {
                            input message: "There have been failures applying patches. Please review the 🚀 output. Fixes can be submitted to origin/${branchName} before continuing.",
                                  ok: "I have fixed them"
                            def conflictExitCode = sh(
                                script: "vpython3 tools/cr/brockit.py lift --to=${params.CHROMIUM_VERSION} --verbose --infra-mode --continue",
                                returnStatus: true
                            )
                            echo "Conflict resolution process exited with code: ${conflictExitCode}"
                            if (conflictExitCode != 0) {
                                echo "Conflict resolution process failed. Determining reason for failure..."
                                failureReason = sh(
                                    script: "vpython3 tools/cr/brockit.py show --reason-for-lift-failure-for=${params.CHROMIUM_VERSION}",
                                    returnStdout: true
                                ).trim()
                                if (!failureReason) {
                                    error("Conflict resolution process failed with exit code ${conflictExitCode}. Check its output.")
                                }
                                echo "Updated failure reason: ${failureReason}"
                            } else {
                                echo "Conflict resolution succeeded."
                                break
                            }
                        }
                    }
                }
            }
        }
        stage('push') {
            steps {
                dir("${BRAVE_CORE_PATH}") {
                    script {
                        sh """
                        echo "Pushing changes to origin/${branchName}..."
                        """
                    }
                }
            }
        }
    }
}

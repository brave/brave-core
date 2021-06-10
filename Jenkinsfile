pipeline {
    agent none
    options {
        ansiColor('xterm')
        timestamps()
    }
    parameters {
        choice(name: 'CHANNEL', choices: ['nightly', 'dev', 'beta', 'release', 'development'])
        choice(name: 'BUILD_TYPE', choices: ['Release', 'Debug'])
        choice(name: 'BUILD_STATUS', choices: ['', 'SUCCESS', 'FAILURE', 'UNSTABLE', 'ABORTED'])
        booleanParam(name: 'TERMINATE_NODE', defaultValue: false)
        booleanParam(name: 'WIPE_WORKSPACE', defaultValue: false)
        booleanParam(name: 'SKIP_INIT', defaultValue: false)
        booleanParam(name: 'DISABLE_SCCACHE', defaultValue: false)
        booleanParam(name: 'SKIP_SIGNING', defaultValue: true)
        booleanParam(name: 'DCHECK_ALWAYS_ON', defaultValue: true)
        string(name: 'NODE_LABEL', defaultValue: '')
        string(name: 'SLACK_NOTIFY', defaultValue: '')
    }
    stages {
        stage('build') {
            agent { label 'master' }
            steps {
                script {
                    REPO = JOB_NAME.substring(0, JOB_NAME.indexOf('-build-pr'))
                    OTHER_REPO = REPO.equals('brave-browser') ? 'brave-core' : 'brave-browser'
                    PLATFORM = JOB_NAME.substring(JOB_NAME.indexOf('-build-pr') + 10, JOB_NAME.indexOf('/PR-'))
                    PIPELINE_NAME = 'pr-brave-browser-' + CHANGE_BRANCH.replace('/', '-') + '-' + PLATFORM

                    if (params.BUILD_STATUS) {
                        if (Jenkins.instance.getItemByFullName(JOB_NAME).getLastBuild().getCause(hudson.model.Cause$UpstreamCause) == null) {
                            echo 'Aborting build as it has been started manually with BUILD_STATUS set'
                            currentBuild.result = 'ABORTED'
                            return
                        }
                        else {
                            echo "Setting other PR build status to ${params.BUILD_STATUS}"
                            currentBuild.result = params.BUILD_STATUS
                            return
                        }
                    }

                    withCredentials([usernamePassword(credentialsId: 'brave-builds-github-token-for-pr-builder', usernameVariable: 'PR_BUILDER_USER', passwordVariable: 'PR_BUILDER_TOKEN')]) {
                        GITHUB_API = 'https://api.github.com/repos/brave'
                        GITHUB_AUTH_HEADERS = [[name: 'Authorization', value: 'token ' + PR_BUILDER_TOKEN]]
                        def prDetails = readJSON(text: httpRequest(url: GITHUB_API + '/' + REPO + '/pulls?head=brave:' + CHANGE_BRANCH, customHeaders: GITHUB_AUTH_HEADERS, quiet: true).content)[0]
                        SKIP = prDetails.draft.equals(true) || prDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/skip') }.equals(1) || prDetails.labels.count { label -> label.name.equalsIgnoreCase("CI/skip-${PLATFORM}") }.equals(1)
                        RUN_NETWORK_AUDIT = prDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/run-network-audit') }.equals(1)
                        RUN_AUDIT_DEPS = prDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/run-audit-deps') }.equals(1)
                        def branchExistsInOtherRepo = httpRequest(url: GITHUB_API + '/' + OTHER_REPO + '/branches/' + CHANGE_BRANCH, validResponseCodes: '100:499', customHeaders: GITHUB_AUTH_HEADERS, quiet: true).status.equals(200)
                        if (branchExistsInOtherRepo) {
                            def otherPrDetails = readJSON(text: httpRequest(url: GITHUB_API + '/' + OTHER_REPO + '/pulls?head=brave:' + CHANGE_BRANCH, customHeaders: GITHUB_AUTH_HEADERS, quiet: true).content)[0]
                            if (otherPrDetails) {
                                env.OTHER_PR_NUMBER = otherPrDetails.number
                                SKIP = SKIP || otherPrDetails.draft.equals(true) || otherPrDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/skip') }.equals(1) || otherPrDetails.labels.count { label -> label.name.equalsIgnoreCase("CI/skip-${PLATFORM}") }.equals(1)
                                RUN_NETWORK_AUDIT = RUN_NETWORK_AUDIT || otherPrDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/run-network-audit') }.equals(1)
                                RUN_AUDIT_DEPS = RUN_AUDIT_DEPS || otherPrDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/run-audit-deps') }.equals(1)
                            }
                        }
                    }

                    if (SKIP && PLATFORM != 'pre-init' && PLATFORM != 'post-init' ) {
                        echo "Aborting build as PRs are either in draft or have a skip label (CI/skip or CI/skip-${PLATFORM})"
                        currentBuild.result = 'SUCCESS'
                        return
                    }

                    for (build in Jenkins.instance.getItemByFullName(JOB_NAME).builds) {
                        if (build.isBuilding() && build.getNumber() < BUILD_NUMBER.toInteger()) {
                            echo 'Aborting older running build ' + build
                            build.doStop()
                        }
                    }

                    jobDsl(scriptText: """
                        pipelineJob('${PIPELINE_NAME}') {
                            // this list has to match the parameters in the Jenkinsfile from devops repo
                            parameters {
                                choiceParam('CHANNEL', ['nightly', 'dev', 'beta', 'release', 'development'])
                                choiceParam('BUILD_TYPE', ['Release', 'Debug'])
                                booleanParam('TERMINATE_NODE', false)
                                booleanParam('WIPE_WORKSPACE', false)
                                booleanParam('SKIP_INIT', false)
                                booleanParam('DISABLE_SCCACHE', false)
                                booleanParam('SKIP_SIGNING', true)
                                booleanParam('DCHECK_ALWAYS_ON', true)
                                booleanParam('RUN_NETWORK_AUDIT', false)
                                booleanParam('RUN_AUDIT_DEPS', false)
                                stringParam('BRANCH', '${CHANGE_BRANCH}')
                                stringParam('NODE_LABEL', '')
                                stringParam('SLACK_NOTIFY', '')
                            }
                            definition {
                                cpsScm {
                                    scm {
                                        git {
                                            remote {
                                                credentials('brave-builds-github-token-for-pr-builder')
                                                github('brave/devops', 'https')
                                            }
                                            branch('master')
                                        }
                                    }
                                    scriptPath("jenkins/jobs/browser/pr-brave-browser-${PLATFORM}.Jenkinsfile")
                                }
                            }
                        }
                    """)

                    params = [
                        string(name: 'CHANNEL', value: params.CHANNEL),
                        string(name: 'BUILD_TYPE', value: params.BUILD_TYPE),
                        booleanParam(name: "TERMINATE_NODE", value: params.TERMINATE_NODE),
                        booleanParam(name: 'WIPE_WORKSPACE', value: params.WIPE_WORKSPACE),
                        booleanParam(name: 'SKIP_INIT', value: params.SKIP_INIT),
                        booleanParam(name: 'DISABLE_SCCACHE', value: params.DISABLE_SCCACHE),
                        booleanParam(name: 'SKIP_SIGNING', value: params.SKIP_SIGNING),
                        booleanParam(name: 'DCHECK_ALWAYS_ON', value: params.DCHECK_ALWAYS_ON),
                        booleanParam(name: 'RUN_NETWORK_AUDIT', value: RUN_NETWORK_AUDIT),
                        booleanParam(name: 'RUN_AUDIT_DEPS', value: RUN_AUDIT_DEPS),
                        string(name: 'BRANCH', value: CHANGE_BRANCH),
                        string(name: 'NODE_LABEL', value: params.NODE_LABEL),
                        string(name: 'SLACK_NOTIFY', value: params.SLACK_NOTIFY)
                    ]

                    currentBuild.result = build(job: PIPELINE_NAME, parameters: params, propagate: false).result
                    if (env.OTHER_PR_NUMBER) {
                        build(job: OTHER_REPO + '-build-pr-' + PLATFORM + '/PR-' + env.OTHER_PR_NUMBER, parameters: [string(name: 'BUILD_STATUS', value: currentBuild.result)], propagate: false)
                    }
                }
            }
        }
    }
    post {
        always {
            node('master') {
                script {
                    sh 'rm -rf .git/index.lock'
                }
            }
        }
    }
}

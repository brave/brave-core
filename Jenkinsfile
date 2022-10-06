def SKIP_SIGNING_DEFAULT = ! JOB_NAME.contains("windows")
pipeline {
    agent none
    options {
        ansiColor('xterm')
        timestamps()
    }
    parameters {
        choice(name: 'CHANNEL', choices: ['nightly', 'dev', 'beta', 'release', 'development'])
        choice(name: 'BUILD_TYPE', choices: ["Static", "Release", "Component", "Debug"])
        booleanParam(name: 'WIPE_WORKSPACE', defaultValue: false)
        booleanParam(name: 'USE_GOMA', defaultValue: true)
        booleanParam(name: 'SKIP_SIGNING', defaultValue: SKIP_SIGNING_DEFAULT)
        booleanParam(name: 'DCHECK_ALWAYS_ON', defaultValue: true)
        string(name: 'DEVOPS_BRANCH', defaultValue: 'master')
        string(name: 'NODE_LABEL', defaultValue: '')
        string(name: 'SLACK_NOTIFY', defaultValue: '')
    }
    stages {
        stage('build') {
            agent { label 'master' }
            steps {
                script {
                    PLATFORM = JOB_NAME.substring(JOB_NAME.indexOf('-build-pr') + 10, JOB_NAME.indexOf('/PR-'))
                    PIPELINE_NAME = 'pr-brave-browser-' + CHANGE_BRANCH.replace('/', '-') + '-' + PLATFORM

                    withCredentials([usernamePassword(credentialsId: 'brave-builds-github-token-for-pr-builder', usernameVariable: 'PR_BUILDER_USER', passwordVariable: 'PR_BUILDER_TOKEN')]) {
                        GITHUB_API = 'https://api.github.com/repos/brave'
                        GITHUB_AUTH_HEADERS = [[name: 'Authorization', value: 'token ' + PR_BUILDER_TOKEN]]
                        CHANGE_BRANCH_ENCODED = java.net.URLEncoder.encode(CHANGE_BRANCH, 'UTF-8')
                        def prDetails = readJSON(text: httpRequest(url: GITHUB_API + '/brave-core/pulls?head=brave:' + CHANGE_BRANCH_ENCODED, customHeaders: GITHUB_AUTH_HEADERS, quiet: true).content)[0]
                        SKIP = (prDetails.draft.equals(true) && prDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/run-draft') }.equals(0)) || prDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/skip') }.equals(1) || prDetails.labels.count { label -> label.name.equalsIgnoreCase("CI/skip-${PLATFORM}") }.equals(1)
                        RUN_NETWORK_AUDIT = prDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/run-network-audit') }.equals(1)
                        RUN_AUDIT_DEPS = prDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/run-audit-deps') }.equals(1)
                        RUN_UPSTREAM_TESTS = prDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/run-upstream-tests') }.equals(1)
                        SKIP_UPSTREAM_TESTS = prDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/skip-upstream-tests') }.equals(1)
                        STORYBOOK = prDetails.labels.count { label -> label.name.equalsIgnoreCase('CI/storybook-url') }.equals(1)
                    }

                    if (SKIP && PLATFORM != 'noplatform') {
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
                                choiceParam('BUILD_TYPE', ["Static", "Release", "Component", "Debug"])
                                booleanParam('WIPE_WORKSPACE', false)
                                booleanParam('USE_GOMA', true)
                                booleanParam('SKIP_SIGNING', ${SKIP_SIGNING_DEFAULT})
                                booleanParam('DCHECK_ALWAYS_ON', true)
                                booleanParam('RUN_NETWORK_AUDIT', false)
                                booleanParam('RUN_AUDIT_DEPS', false)
                                booleanParam('RUN_UPSTREAM_TESTS', false)
                                booleanParam('SKIP_UPSTREAM_TESTS', false)
                                booleanParam('STORYBOOK', false)
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
                                            branch('${params.DEVOPS_BRANCH}')
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
                        booleanParam(name: 'WIPE_WORKSPACE', value: params.WIPE_WORKSPACE),
                        booleanParam(name: 'USE_GOMA', value: params.USE_GOMA),
                        booleanParam(name: 'SKIP_SIGNING', value: params.SKIP_SIGNING),
                        booleanParam(name: 'DCHECK_ALWAYS_ON', value: params.DCHECK_ALWAYS_ON),
                        booleanParam(name: 'RUN_NETWORK_AUDIT', value: RUN_NETWORK_AUDIT),
                        booleanParam(name: 'RUN_AUDIT_DEPS', value: RUN_AUDIT_DEPS),
                        booleanParam(name: 'RUN_UPSTREAM_TESTS', value: RUN_UPSTREAM_TESTS),
                        booleanParam(name: 'SKIP_UPSTREAM_TESTS', value: SKIP_UPSTREAM_TESTS),
                        booleanParam(name: 'STORYBOOK', value: STORYBOOK),
                        string(name: 'BRANCH', value: CHANGE_BRANCH),
                        string(name: 'NODE_LABEL', value: params.NODE_LABEL),
                        string(name: 'SLACK_NOTIFY', value: params.SLACK_NOTIFY)
                    ]

                    currentBuild.result = build(job: PIPELINE_NAME, parameters: params, propagate: false).result
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

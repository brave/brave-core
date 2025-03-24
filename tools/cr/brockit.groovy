pipeline {
    agent any
    environment {
        CHROMIUM_SRC_PATH = "brave-browser/src/"
        BRAVE_CORE_PATH = "brave-browser/src/brave/"
        PATH = "${WORKSPACE}/${BRAVE_CORE_PATH}/vendor/depot_tools:${PATH}"
    }
    stages {
        stage('Initialise') {
            steps {
                dir("${BRAVE_CORE_PATH}") {
                    sh """
                    git -C ../ fetch origin tag 136.0.7081.1
                    git -C ../ checkout --force FETCH_HEAD
                    npm run init
                    """
                }
            }
        }
    }
}

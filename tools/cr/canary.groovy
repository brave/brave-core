def canaryStages() {
    stage('Initialise') {
        dir("${BRAVE_CORE_PATH}") {
            sh "npm install"
            sh "npm run init"
        }
    }
}

// Ensure the script returns itself for `load()`
return this

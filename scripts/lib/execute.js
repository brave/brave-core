var execSync = require('child_process').execSync

module.exports = function (cmds, env, cb) {
  if (!env) {
    env = {}
  }
  var cmd = ''
  if (!Array.isArray(cmds)) {
    cmds = [cmds]
  }
  cmd += cmds.join('&&')
  console.log(cmd)

  for (var key in env) {
    if (env.hasOwnProperty(key)) {
      process.env[key] = env[key]
    }
  }

  var r = execSync(cmd, {
    env: process.env
  }, function (err) {
    if (cb) {
      cb(err)
    }
  })
  r.stdout.pipe(process.stdout)
  r.stderr.pipe(process.stderr)
}

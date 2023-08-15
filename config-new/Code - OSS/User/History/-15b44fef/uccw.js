var express = require('express') // Express web server framework

var cookieParser = require('cookie-parser')
var https = require('https')
var cors = require('cors')
var bodyParser = require('body-parser')
var commands = require('./src/commands.js')
var midiChartGenerator = require('./src/midiChartGenerator.js')

var app = express().use('*', cors())

app.use(express.static(__dirname + '/public'))//.use(cookieParser())
var sslPath = '/etc/letsencrypt/live/linux.jamstik.com/'
const fs = require('fs')

var globalOptions = {
  key: fs.readFileSync(sslPath + 'privkey.pem'),
  cert: fs.readFileSync(sslPath + 'fullchain.pem')
}

// make it accept json from post requests
// I had to do this because posting json wasn't working
app.use(
  bodyParser.urlencoded({
    extended: true
  })
)
app.use(bodyParser.json())

// commands requests
app.post('/createJson', commands.createJson)
app.get('/modifyJson', commands.modifyJson)
app.get('/getMeDirectoryContents', commands.getMeDirectoryContents)
app.get('/whatSongsAreInTab', commands.whatSongsAreInTab)
app.post('/uploadZip', commands.uploadZip)
app.get(
  '/whatIsMostRecentPlayPortalDeployVersionNumber',
  commands.whatIsMostRecentPlayPortalDeployVersionNumber
)
app.get('/midiChartGenerator', midiChartGenerator.begin)
app.get('/completeSongUploadProcess', commands.completeSongUploadProcess)
app.get('/delete', commands.delete)
app.get('/latestPluginVersion', commands.latestPlugin)
app.get('/latestCreatorSoundLibraryInfo', commands.latestCreatorSoundLibraryInfo)
app.get('/generateFirmwareFiles',commands.generateFirmwareFiles)


var server = https.createServer(globalOptions, app)
var io = require('socket.io').listen(server)
server.listen(443, function (err) {
  if (err) return cb(err)

  // Find out which user used sudo through the environment variable
  var uid = parseInt(process.env.SUDO_UID)
  // Set our server's uid to that user
  if (uid) process.setuid(uid)
  console.log("Server's UID is now " + process.getuid())
})

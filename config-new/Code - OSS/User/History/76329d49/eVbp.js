var fs = require("fs");
var multer = require("multer");
var unzip = require("unzipper");
var mv = require("mv");
var mkpath = require("mkpath");
var rimraf = require("rimraf");

var storage = multer.diskStorage({
  destination: function (req, file, cb) {
    cb(null, "./public/stage");
  },
  filename: function (req, file, cb) {
    // cb(null, Date.now() + '-' + file.originalname)
    cb(null, file.originalname);
  },
});

var uploadZipMulter = multer({ storage: storage }).single("file");

module.exports = {
  // should look something like this with the location request:
  // "./public/files/play-portal/charts/"
  getMeDirectoryContents: function (req, res) {
    const directoryInQuestion = req.query.payload;
    console.log("\n", "getMeDirectoryContents => ", directoryInQuestion);
    fs.readdir(directoryInQuestion, (err, files) => {
      // console.log("what's inside the directory:", files);
      res.send(files);
    });
  },
  //inside data ajax request, send something like this:
  //{file: './public/files/play-portal/charts/00000/metaData.json', modifyOrReplace: "modify", payload: JSON.stringify({deploymentStatus: "staged"})}
  //I made payload a string because was having dumb issues with stringified numeric keys
  modifyJson: function (req, res) {
    const fileInQuestion = req.query.file;
    console.log("\n", "modifyJson => ", fileInQuestion);
    console.log("them there", req.query.payload)
    let file = fs.readFileSync(`${fileInQuestion}`);
    let fileToModify = JSON.parse(file);
    const thingsToModify =
      req.query.modifyOrReplace == "modify"
        ? Object.keys(JSON.parse(req.query.payload)).map((k) => ({
            key: k,
            value: JSON.parse(req.query.payload)[k],
          }))
        : [];

    thingsToModify.forEach((thing) => {
      fileToModify[thing.key] = thing.value;
    });

    fs.writeFile(
      fileInQuestion,
      req.query.modifyOrReplace == "replace"
        ? JSON.stringify(req.query.payload, null, 2)
        : JSON.stringify(fileToModify, null, 2),
      function (err) {
        if (err) return res.send(false);
        res.send(true);
      }
    );
  },
  whatSongsAreInTab: function (req, res) {
    console.log("\n", "whatSongsAreInTab");
    fs.readdir("./public/files/play-portal/charts/", (err, files) => {
      res.send(files);
    });
  },
  createJson: function (req, res) {
    const path = req.body.payload.path;
    const json = req.body.payload.json;
    console.log("\n", "createJson => ", "path: ", path, " json: ", json);
    fs.writeFile(path, JSON.stringify(json, null, 2), function (err) {
      if (err) {
        res.send(false);
        return console.log(err);
      }
      res.send(true);
    });
  },
  //  {typeOfJamstik: "Studio", whichRevisions: CAN BE EITHER [] for all or ["2.52", "2.60"] for each}
  generateFirmwareFiles: function (req, res) {
    console.log("generatefirmwarefile called", JSON.stringify(req.query));
    const typeOfJamstik = req.query.typeOfJamstik;
    console.log("type of jamstik:", typeOfJamstik)
    const whichFiles =
      req.query.whichRevisions == "all"
        ? fs.readdirSync(`./public/files/firmware/raw/${typeOfJamstik}`)
        : req.query.whichRevisions;
    console.log("whichfiles is", whichFiles);
    const allRawFiles = whichFiles.map((fileName) => {
      const file = fs.readFileSync(
        `./public/files/firmware/raw/${typeOfJamstik}/${fileName}`,
        "utf8"
      );
      const splitFile = file.split("\n").map((line) =>
        line
          .replace(/(\r\n|\n|\r)/gm, "")
          .replace(":", "")
          .split('')
          .map((character) => character.charCodeAt(0))
      );
      console.log("split the file:", fileName);
      return splitFile;
    });
    whichFiles.forEach((fileName, index) => {
      console.log("writing to", fileName)
      const fileNameWithoutExtension = fileName.replace(/\.[^/.]+$/, "");
      fs.writeFileSync(
        `./public/files/firmware/generatedJsons/${typeOfJamstik}/${fileNameWithoutExtension}.json`,
        JSON.stringify(allRawFiles[index], null, 2)
      );
      console.log("successfully wrote to", fileName)
    });
    res.send(true)
  },
  uploadZip: function (req, res) {
    console.log("\n", "uploadZip");
    // put the zip in the stage area
    uploadZipMulter(req, res, function (err) {
      if (err instanceof multer.MulterError) {
        return res.status(500).json(err);
      } else if (err) {
        return res.status(500).json(err);
      }
      return res.status(200).send(req.file);
    });
  },
  completeSongUploadProcess: function (req, res) {
    // 0001_Svenska_Julen-ar-har.zip
    // const referenceFileName = "whydoesntthiswork.zip"
    // 0001_Svenska_Julen-ar-har
    const referenceFileName = req.query.zipFileName;
    const referenceFile = referenceFileName.split(".")[0];
    // const referenceFile = "whydoesntthiswork"
    // zip
    const referenceFileType = referenceFileName.split(".")[1];
    console.log("\n", "completeSongUploadProcess => ", referenceFileName);
    // const referenceFileType = "zip"
    // fs.createReadStream('./public/stage/zeep.zip').pipe(unzip.Extract({ path: './public/stage/noewdirec' }));
    // console.log('the great thing', req.file.filename)
    // make the directory
    mkpath(`./public/stage/${referenceFile}`, function (err1) {
      console.log("errorz", err1);
      //begin attempt to unzip
      if (err1) {
        console.error("err1", err1);
      }
      // making sure the directory was successful
      // else {
      console.log("made directory in stage");
      // unzip into the directory
      const stream = fs
        .createReadStream(`./public/stage/${referenceFileName}`)
        .pipe(unzip.Extract({ path: `./public/stage/${referenceFile}/` }))
        .on("close", function (err2) {
          fs.readdir(
            `./public/stage/${referenceFile}/`,
            (err, userUploadedTheseFilesInsideTheZip) => {
              console.log("in close");
              // delete the zip file since we're done with it
              fs.unlink(`./public/stage/${referenceFileName}`, (err3) => {
                if (err3) {
                  console.error("err3", err3);
                }
                console.log("about to move folder to charts");
                // now that the zip file is deleted, we can move the extracted folder to the right spot
                mv(
                  `./public/stage/${referenceFile}/`,
                  `./public/files/play-portal/charts/${referenceFile}`,
                  { mkdirp: true },
                  function (err4) {
                    // callback for when the move finishes
                    // this part is pretty much the last part
                    console.log("put it in there");
                    res.send(userUploadedTheseFilesInsideTheZip);
                  }
                );
              });
            }
          );
        })
        .on("error", function (err5) {
          console.log("ERROR WHILE TRYING TO UNZIP: ", err5);
        });
      // }
      //end attempt to unzip
    });
  },
  whatIsMostRecentPlayPortalDeployVersionNumber: function (req, res) {
    const file = fs.readFileSync(`./public/files/json/portal.json`);
    const latestPlayPortalVersion = JSON.parse(file).version;
    res.send(latestPlayPortalVersion);
  },
  latestPlugin: function (req, res) {
    const file = fs.readFileSync(`./public/files/json/plugin.json`);
    const pluginInfo = JSON.parse(file);
    res.send(pluginInfo);
  },
  latestCreatorSoundLibraryInfo: function (req, res) {
    const file = fs.readFileSync(
      `./public/files/json/soundLibraryMetaData.json`
    );
    const soundLibraryInfo = JSON.parse(file);
    res.send(soundLibraryInfo);
  },
  delete: function (req, res) {
    const fileOrFolderInQuestion = req.query.payload;
    console.log("\n", "delete => ", fileOrFolderInQuestion);
    rimraf(fileOrFolderInQuestion, fs, function (err) {
      if (err) {
        res.send(false);
      } else {
        res.send(true);
      }
    });
  },
};

/*

        if (err1) {
          console.error('err1', err1)
        }
        // making the directory was successful
        // else {
        console.log('made directory in stage')
        // unzip into the directory
        const stream = fs.createReadStream(`./public/stage/${referenceFileName}/`)
        stream.pipe(unzipper.Extract({ path: `./public/stage/${referenceFile}/.` }))

        stream.on('close', function (err2) {
          // delete the zip file since we're done with it
          fs.unlink(`./public/stage/${referenceFileName}`, err3 => {
            if (err3) {
              console.error('err3', err3)
            }
            console.log('about to move folder to charts')
            // now that the zip file is deleted, we can move the extracted folder to the right spot
            mv(
              `./public/stage/${referenceFile}/`,
              `./public/files/play-portal/charts/${referenceFile}`,
              { mkdirp: true },
              function (err4) {
                // callback for when the move finishes
                // this part is pretty much the last part
                console.log('put it in there')
              }
            )
          })
        })
        // }
*/

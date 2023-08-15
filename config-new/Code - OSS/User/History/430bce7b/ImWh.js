/*

GOES THROUGH ALL OF THE FILES INSIDE /charts AND CREATES CHARTS FOR EACH OF THEM

*/

var fs = require('fs')
var _ = require('lodash')
var parseMidi = require('midi-file').parseMidi
var writeMidi = require('midi-file').writeMidi
var Midi = require('@tonejs/midi')
const directoryForCharts = './public/files/play-portal/charts/'

const modifierKeyDictionary = {
  24: 'hold',
  25: 'hammer',
  26: 'pitchBend',
  // THESE CAN ONLY BE ON THE GLOBAL MODIFIER CHANNEL
  27: 'strumDown',
  28: 'strumUp',
  29: 'mutedStrumDown',
  30: 'mutedStrumUp'
}

module.exports = {
  begin: function (req, res) {
    let messages = ''

    //take argument from what you wanna regenerate
    const generate = req.query.generate

    function goThroughSelectedFiles (files) {
      let errorWasFound = false
      const totalNumberOfCharts = files.length
      let currentChartProcessing = 0
      files.forEach(chartFolder => {
        // this is the final chart that we are building that we will write to a file or something
        let generatedChart = {}
        console.log(chartFolder)
        messages = messages + chartFolder + '\n'
        const midiData = fs.readFileSync(
          directoryForCharts + chartFolder + '/chart.mid'
        )
        try {
          const metaData = JSON.parse(
            fs.readFileSync(directoryForCharts + chartFolder + '/metaData.json')
          )
          console.log('    =>', metaData.title)
          messages = messages + '    =>' + metaData.title + '\n'
          generatedChart = metaData

          const midi = Object.assign({}, new Midi(midiData))
          //console.log('MIDI is:', JSON.stringify(midi, null, 2))

          let flattenedMidiNotes = []
          // the file name decoded from the first track
          const name = midi.name
          // get the tracks
          // console.log(JSON.stringify(midi));
          //  var json = JSON.stringify(midi);
          // first we gotta sort the garbage that we got
          // FL studio is really bad about this and uses the "name" property to specify different channels

          const usedTracks = midi.tracks
            .reduce(
              (accumulator, track) => [
                ...accumulator,
                parseInt(track.name[track.name.length - 1], 10)
              ],
              []
            )
            .map(n => n)

          const missingTracks = [...Array(7)]
            .map((n, i) => i + 1)
            .filter(num => !usedTracks.includes(num))
          missingTracks.forEach(trackNumber =>
            midi.tracks.push({
              channel: trackNumber,
              controlChanges: {},
              instrument: {
                family: 'piano',
                name: 'acoustic grand piano',
                number: 0
              },
              name: `${trackNumber}`,
              notes: []
            })
          )

          midi.tracks.sort((a, b) => {
            return (
              parseInt(a.name[a.name.length - 1]) -
              parseInt(b.name[b.name.length - 1])
            )
          })

          fs.writeFileSync(
            directoryForCharts + chartFolder + '/rawMidi.json',
            JSON.stringify(midi, null, 2),
            err => {
              if (err) console.log(err)
              console.log('Successfully written rawMidi to file.')
            }
          )

          // this sets the channel properly because fl studio is dumb
          // also it flattens the midi notes
          // this k here is just so we speed up not having to get the length of flattenedMidiNotes for every note
          let k = 0
          for (let i = 0; i < midi.tracks.length; i++) {
            midi.tracks[i].channel = i
            for (let j = 0; j < midi.tracks[i].notes.length; j++) {
              // console.log("settings this track");
              midi.tracks[i].notes[j].channel = i
              // WHY DID THESE THREE THINGS GO AWAY??? i'm so mad
              const missingDuration = midi.tracks[i].notes[j].duration
              const missingName = midi.tracks[i].notes[j].name
              const missingTime = midi.tracks[i].notes[j].time

              if (
                // 6th channel is always reserved for global modifier keys
                i != 6 &&
                // makes sure that the note in question is above the lowest point of the string
                // this will result in ensuring it's NOT a single channel modifier key basically
                // there is no upper limit
                midi.tracks[i].notes[j].midi >=
                  metaData.tuning[
                    5 -
                      i /* everytime we deal with tuning, gotta flip the direction */
                  ]
              ) {
                flattenedMidiNotes.push(
                  Object.assign({}, midi.tracks[i].notes[j])
                )
                flattenedMidiNotes[k].duration = missingDuration
                flattenedMidiNotes[k].name = missingName
                flattenedMidiNotes[k].time = missingTime
                flattenedMidiNotes[k].stringNumber =
                  5 - flattenedMidiNotes[k].channel
                flattenedMidiNotes[k].fretNumber =
                  flattenedMidiNotes[k].midi -
                  metaData.tuning[flattenedMidiNotes[k].stringNumber]
                delete flattenedMidiNotes[k].ticks
                delete flattenedMidiNotes[k].durationTicks
                delete flattenedMidiNotes[k].noteOffVelocity
                delete flattenedMidiNotes[k].velocity
                flattenedMidiNotes[k].noteNumber = flattenedMidiNotes[k].midi
                delete flattenedMidiNotes[k].midi
                delete flattenedMidiNotes[k].channel

                // ONLY increment k when pushing to the array
                k++
              }
            }
          }
          // now that we've flattened the notes, we need to sort them by time

          flattenedMidiNotes.sort((a, b) => {
            return a.time - b.time
          })
          flattenedMidiNotes.push(flattenedMidiNotes[0])
          // flattenedMidiNotes[flattenedMidiNotes.length-1];

          // prepare to group notes together
          let groupedChordallyMidiNotes = []
          // GROUP the notes that are being played at the same time
          for (let i = 0; i < flattenedMidiNotes.length; i++) {
            // look one ahead and see if that's the same time
            // this variable is named badly
            // because we use it even if it's a single note to initialize that single note
            let nextNoteIsPartOfChord = true
            let initializedTheChordInQuestion = false
            while (nextNoteIsPartOfChord) {
              if (initializedTheChordInQuestion) {
                groupedChordallyMidiNotes[
                  groupedChordallyMidiNotes.length - 1
                ].notes.push(flattenedMidiNotes[i])
                nextNoteIsPartOfChord = false
              }
              // this if statement just ensures that we're not out of range
              if (i < flattenedMidiNotes.length - 1) {
                if (
                  flattenedMidiNotes[i + 1].time === flattenedMidiNotes[i].time
                ) {
                  nextNoteIsPartOfChord = true
                  // group together the notes
                  if (!initializedTheChordInQuestion) {
                    groupedChordallyMidiNotes.push({
                      centerOfTimingWindow: flattenedMidiNotes[i].time,
                      isChord: true,
                      notes: [flattenedMidiNotes[i]]
                    })
                    initializedTheChordInQuestion = true
                  }
                  i++
                } else {
                  // SINGLE NOTE, NOT A CHORD
                  nextNoteIsPartOfChord = false
                  // no need to modify initializedTheChordInQuestion because it makes no sense for single notes
                  if (!initializedTheChordInQuestion) {
                    groupedChordallyMidiNotes.push({
                      centerOfTimingWindow: flattenedMidiNotes[i].time,
                      isChord: false,
                      notes: [flattenedMidiNotes[i]]
                    })
                  }
                }
              }
              if (i === flattenedMidiNotes.length - 1) {
                nextNoteIsPartOfChord = false
              }
            }
          }

          // know the range of the modifier keys
          // this is so that we can check to see if things in the future are within them
          // so that we know they need to be modified
          let rangeOfModifiers = {
            global: {},
            channelSpecific: [{}, {}, {}, {}, {}, {}]
          }
          let flattenedRangeOfModifiers = []
          for (
            let i = 0;
            i < Object.values(modifierKeyDictionary).length;
            i++
          ) {
            rangeOfModifiers.global[
              Object.values(modifierKeyDictionary)[i]
            ] = []
            for (let j = 0; j < 6; j++) {
              rangeOfModifiers.channelSpecific[j][
                Object.values(modifierKeyDictionary)[i]
              ] = []
            }
          }
          // globals first, let's take care of those
          if (midi.tracks.length >= 7) {
            if (midi.tracks[6].hasOwnProperty('notes')) {
              for (let i = 0; i < midi.tracks[6].notes.length; i++) {
                const specificModifierNote = midi.tracks[6].notes[i]
                // NOT FLATTENED
                rangeOfModifiers.global[
                  modifierKeyDictionary[`${specificModifierNote.midi}`]
                ].push({
                  start: specificModifierNote.time,
                  end: specificModifierNote.time + specificModifierNote.duration
                })
                // FLATTENED
                flattenedRangeOfModifiers.push({
                  start: specificModifierNote.time,
                  end:
                    specificModifierNote.time + specificModifierNote.duration,
                  timbrality: 'global',
                  type: modifierKeyDictionary[`${specificModifierNote.midi}`]
                })
              }
            }
          }
          // we took care of the globals, now we can do channel specific modifiers
          // reason being treating different is because we have a max midi value to test against
          /*
          for (let i = 0; i < 5; i++) {
            for (let j = 0; j < midi.tracks[i].notes.length; j++) {
              // make sure the note in question is actually a modifier note
              if (midi.tracks[i].notes[j].midi < metaData.tuning[5 - i]) {
                //console.log("them there gun", i, "yes", midi.tracks[i], metaData.tuning[5-i])
                const specificModifierNote = midi.tracks[i].notes[j]
                // NOT FLATTENED
                //console.log("NOTE:", specificModifierNote)
                rangeOfModifiers.channelSpecific[5 - i][
                  modifierKeyDictionary[`${specificModifierNote.midi}`]
                ].push({
                  start: specificModifierNote.time,
                  end: specificModifierNote.time + specificModifierNote.duration
                })
                // FLATTENED
                flattenedRangeOfModifiers.push({
                  start: specificModifierNote.time,
                  end:
                    specificModifierNote.time + specificModifierNote.duration,
                  timbrality: 'channelSpecific',
                  // this "stringNumber" thing only exists if timbrality is equal to channelSpecific, not global
                  stringNumber: 5 - i,
                  type: modifierKeyDictionary[`${specificModifierNote.midi}`]
                })
              }
            }
          }
          */

          // console.log(JSON.stringify(flattenedRangeOfModifiers, null, 2));

          // ADD known modifiers to notes!

          for (let i = 0; i < groupedChordallyMidiNotes.length; i++) {
            // groupedChordallyMidiNotes[i].modifiers = [];
            /*
          switch (groupedChordallyMidiNotes[i].isChord) {
            case true:
              for (
                let j = 0;
                j < groupedChordallyMidiNotes[i].notes.length;
                j++
              ) {
                for (let k = 0; k < flattenedRangeOfModifiers.length; k++) {
                  if (
                    flattenedRangeOfModifiers[k].timbrality === "channelSpecific"
                  ) {
                    //do channel specific modifiers here
                  }
                }
              }
              break;
            case false:
              break;
          }
          */
            for (let j = 0; j < flattenedRangeOfModifiers.length; j++) {
              // APPLY the found modifier
              for (
                let k = 0;
                k < groupedChordallyMidiNotes[i].notes.length;
                k++
              ) {
                if (
                  !groupedChordallyMidiNotes[i].notes[k].hasOwnProperty(
                    'modifiers'
                  )
                ) {
                  groupedChordallyMidiNotes[i].notes[k].modifiers = []
                }
                if (
                  groupedChordallyMidiNotes[i].centerOfTimingWindow >=
                    flattenedRangeOfModifiers[j].start &&
                  groupedChordallyMidiNotes[i].centerOfTimingWindow <
                    flattenedRangeOfModifiers[j].end
                ) {
                  switch (flattenedRangeOfModifiers[j].timbrality) {
                    case 'channelSpecific':
                      if (
                        groupedChordallyMidiNotes[i].notes[k].stringNumber ===
                        flattenedRangeOfModifiers[j].stringNumber
                      ) {
                        groupedChordallyMidiNotes[i].notes[k].modifiers.push(
                          flattenedRangeOfModifiers[j].type
                        )
                      }
                      break
                    default:
                    case 'global':
                      groupedChordallyMidiNotes[i].notes[k].modifiers.push(
                        flattenedRangeOfModifiers[j].type
                      )
                      break
                  }
                }
              }
            }
          }

          // This for loop started out as a quick fix for isaac
          // But soon grew to do more than that, including naming the chords from metaData's chord dictionary
          // It also figures out if the chord grid needs to be repeated or whatever: repeatOfPreviousChord
          // Isaac said that the notes were out of order in the chord array when isChord is true
          // this is a fix for that
          //
          for (let i = 0; i < groupedChordallyMidiNotes.length; i++) {
            // fixed sorting for notes by string
            if (groupedChordallyMidiNotes[i].isChord) {
              groupedChordallyMidiNotes[i].notes.sort((a, b) => {
                return a.stringNumber - b.stringNumber
              })

              // these get turned to false when it finds notes on that string
              // the remaining ones that are still true are the x's of the chord grid
              // eww mutable i hate this code, at least i put lots of comments
              // let's implement doNotPlayTheseStrings
              let stringsStillRemaining = [true, true, true, true, true, true]
              for (
                let j = 0;
                j < groupedChordallyMidiNotes[i].notes.length;
                j++
              ) {
                stringsStillRemaining[
                  groupedChordallyMidiNotes[i].notes[j].stringNumber
                ] = false
              }
              groupedChordallyMidiNotes[
                i
              ].doNotPlayTheseStrings = stringsStillRemaining.reduce(
                (accumulator, currentValue, currentIndex) => {
                  return currentValue
                    ? [...accumulator, currentIndex]
                    : [...accumulator]
                },
                []
              )
              // wow that was annoying, we implemented doNotPlayTheseStrings
              // time to name the chord, j is free to use as variable, but i isn't
              let arrayToCompareAgainstDictionary = []
              for (
                let j = 0;
                j < groupedChordallyMidiNotes[i].notes.length;
                j++
              ) {
                arrayToCompareAgainstDictionary[
                  groupedChordallyMidiNotes[i].notes[j].stringNumber
                ] = groupedChordallyMidiNotes[i].notes[j].fretNumber
              }
              for (
                let j = 0;
                j < groupedChordallyMidiNotes[i].doNotPlayTheseStrings.length;
                j++
              ) {
                arrayToCompareAgainstDictionary[
                  groupedChordallyMidiNotes[i].doNotPlayTheseStrings[j]
                ] = 'x'
              }
              // now arrayToCompareAgainstDictionary is ready
              // wow i hate the mutable world so much
              // bad jimmy
              // never again
              // i say that but i keep doing this garbage
              let didWeFindADictionaryEntryForIt = false
              dictionaryLoop: for (
                let j = 0;
                j < metaData.chordDictionary.length;
                j++
              ) {
                if (
                  _.isEqual(
                    arrayToCompareAgainstDictionary,
                    metaData.chordDictionary[j].notes
                  )
                ) {
                  // this closure means we found a dictionary entry for it! Let's save the name of it
                  groupedChordallyMidiNotes[i].chordName =
                    metaData.chordDictionary[j].name
                  groupedChordallyMidiNotes[
                    i
                  ].anotherWayToLookAtTheChord = arrayToCompareAgainstDictionary
                  didWeFindADictionaryEntryForIt = true
                  break dictionaryLoop
                } else {
                  groupedChordallyMidiNotes[i].chordName = ''
                }
              }
              // now that we successfully named the chord, let's compare it with the previous and see if it is a repeat
              // but first, let's take care of the very very first one
              if (i == 0 && groupedChordallyMidiNotes[0].isChord) {
                groupedChordallyMidiNotes[0].repeatOfPreviousChord = false
              }
              // alright, now let's start at i=1 and go ahead
              if (i > 0) {
                if (i > 0 && groupedChordallyMidiNotes[i].isChord) {
                  groupedChordallyMidiNotes[i].repeatOfPreviousChord = false
                  if (
                    groupedChordallyMidiNotes[i - 1].isChord &&
                    _.isEqual(
                      groupedChordallyMidiNotes[i].anotherWayToLookAtTheChord,
                      groupedChordallyMidiNotes[i - 1]
                        .anotherWayToLookAtTheChord
                    )
                  ) {
                    // it's a repeat!
                    groupedChordallyMidiNotes[i].repeatOfPreviousChord = true
                  }
                }
              }
              // FINISHED: implementing repeatOfPreviousChord

              // let's implement minimumFretOfChordGrid
              const arrayOfFretNumbers = groupedChordallyMidiNotes[i].notes
                .map(n => n.fretNumber)
                .filter(n => n > 0)
              const minFret = Math.min(...arrayOfFretNumbers)
              const maxFret = Math.max(...arrayOfFretNumbers)
              groupedChordallyMidiNotes[i].minimumFretOfChordGrid =
                maxFret > 4 ? minFret : 1
              // Now normally since I'm still acting on groupedChordallyMidiNotes,
              // I would just continue within this for loop
              // But see we need to look for one index ahead when creating timeUntilChordChanges,
              // We're gonna have to leave the for loop and do it again
              // Ok let's jump over
            }
            // BOING
          }
          // BOING
          // We made it! We left the for loop
          // Time to make the same for loop again
          for (let i = 0; i < groupedChordallyMidiNotes.length; i++) {
            // This implements timeUntilChordChanges
            if (groupedChordallyMidiNotes[i].isChord) {
              // if the next "vertical note group", so to speak, is a chord:
              const thisChordCenterOfTimingWindow =
                groupedChordallyMidiNotes[i].centerOfTimingWindow
              // I set it to something but technically it doesn't have to be initially set
              // will be mutated to be correct
              let whenTheChordStops = thisChordCenterOfTimingWindow
              let nextVerticalNoteGroupIsTheSameChord = true
              let indexForLookingAhead = 1
              while (nextVerticalNoteGroupIsTheSameChord) {
                // make sure we're in bounds in the big for loop
                if (
                  i + indexForLookingAhead <
                  groupedChordallyMidiNotes.length
                ) {
                  if (
                    groupedChordallyMidiNotes[i + indexForLookingAhead].isChord
                  ) {
                    if (
                      groupedChordallyMidiNotes[i + indexForLookingAhead]
                        .repeatOfPreviousChord
                    ) {
                      indexForLookingAhead++
                    }
                    // A chord, but not the SAME chord
                    else {
                      nextVerticalNoteGroupIsTheSameChord = false
                    }
                  }
                  // Not a chord, like single notes, so yeah, definitely not the same chord
                  else {
                    nextVerticalNoteGroupIsTheSameChord = false
                  }
                }
                // if we're towards the end of the song
                // we can't check against further notes
                // just give up, leave this while loop
                else {
                  nextVerticalNoteGroupIsTheSameChord = false
                }
              }
              // we have the right indexForLookingAhead value now

              // this case is for if it didn't find any repeats
              if (indexForLookingAhead == 1) {
                // we look at the duration of the chord, if it has a hold modifier, otherwise just set it to zero
                whenTheChordStops = groupedChordallyMidiNotes[
                  i
                ].notes[0].modifiers.includes('hold')
                  ? groupedChordallyMidiNotes[i].notes.reduce(
                      (accumulator, currentValue) =>
                        Math.min(accumulator, currentValue.duration),
                      groupedChordallyMidiNotes[i].notes[0].duration
                    ) + thisChordCenterOfTimingWindow
                  : thisChordCenterOfTimingWindow
              }
              // if we DID find a repeat
              else {
                whenTheChordStops = groupedChordallyMidiNotes[
                  i + indexForLookingAhead - 1
                ].notes[0].modifiers.includes('hold')
                  ? groupedChordallyMidiNotes[
                      i + indexForLookingAhead - 1
                    ].notes.reduce(
                      (accumulator, currentValue) =>
                        Math.min(accumulator, currentValue.duration),
                      groupedChordallyMidiNotes[i].notes[0].duration
                    ) +
                    groupedChordallyMidiNotes[i + indexForLookingAhead - 1]
                      .centerOfTimingWindow
                  : groupedChordallyMidiNotes[i + indexForLookingAhead - 1]
                      .centerOfTimingWindow
              }
              // now let's put that final value in the groupedChordallyMidiNotes
              /*
            console.log(
              'when the chord stops: ',
              whenTheChordStops,
              " , this chord's center of timing window: ",
              thisChordCenterOfTimingWindow,
              " , index for looking ahead: ",
              indexForLookingAhead
            )
            */
              if (!groupedChordallyMidiNotes[i].repeatOfPreviousChord) {
                groupedChordallyMidiNotes[i].timeUntilChordChanges =
                  whenTheChordStops - thisChordCenterOfTimingWindow
              }
            }
          }

          const entireChart = {
            metaData: metaData,
            chart: groupedChordallyMidiNotes
          }

          var jsonPretty = JSON.stringify(entireChart, null, 2)
          // console.log(jsonPretty);
          midi.tracks.forEach(track => {
            // tracks have notes and controlChanges

            // notes are an array
            const notes = track.notes
            notes.forEach(note => {
              // note.midi, note.time, note.duration, note.name
            })

            // the control changes are an object
            // the keys are the CC number
            track.controlChanges[64]
            // they are also aliased to the CC number's common name (if it has one)
            // i commented this out because it was causing an error
            /*
          track.controlChanges.sustain.forEach(cc => {
            // cc.ticks, cc.value, cc.time
          });
          */

            // the track also has a channel and instrument
            // track.instrument.name
          })

          fs.writeFileSync(
            directoryForCharts + chartFolder + '/chart.json',
            jsonPretty,
            err => {
              if (err) console.log(err)
              console.log('Successfully written chart to file.')
            }
          )
          console.log('...')
          console.log('')
          currentChartProcessing++
        } catch (chartGenerationError) {
          console.log('CHART UPLOAD ERROR', chartGenerationError)
          errorWasFound = true
          res.send(
            JSON.stringify(
              chartGenerationError,
              Object.getOwnPropertyNames(chartGenerationError),
              2
            )
          )
        }
        if (currentChartProcessing === totalNumberOfCharts && !errorWasFound) {
          res.send(false)
        }
      })
    }
    if (typeof generate === 'string' && generate === 'all') {
      fs.readdir(directoryForCharts, (err, files) => {
        goThroughSelectedFiles(files)
      })
    } else if (typeof generate === 'object' && generate.length !== 0) {
      goThroughSelectedFiles(generate)
    }
  }
}

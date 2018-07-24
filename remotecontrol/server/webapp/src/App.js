import React, { Component } from 'react';
import { BrowserRouter as Router, Route} from "react-router-dom";
import io from 'socket.io-client';
import './App.css';
//import { Configuration} from './Configuration.js';
import { Cockpit } from './Cockpit.js';
import { RobotDisplay} from './RobotDisplay.js';


function handleGetUserMediaError(error) {
  if (error.name === 'OverconstrainedError') {
    console.error('navigator.getUserMedia() threw an OverconstrainedError. constraint:' + error.constraint + '. message:' + error.message);
  } else {
    console.error('navigator.getUserMedia error: ', error);
    alert('navigator.getUserMedia error: ' + error);
  }
}


function handleEnumerateDevicesError(error) {
  if (error.name === 'OverconstrainedError') {
    console.error('navigator.mediaDevices.enumerateDevices() threw an OverconstrainedError. constraint:' + error.constraint + '. message:' + error.message);
  } else {
    console.error('navigator.mediaDevices.enumerateDevices() error: ', error);
    alert('navigator.mediaDevices.enumerateDevices() error: ' + error);
  }
}


class App extends Component {
  constructor(props) {
    super(props);

    const urlParams = new URLSearchParams(window.location.search);

    const isRobot = (urlParams.get("isRobot") === "true");
    let selectedVideoSource = null;
    let selectedAudioSource = null;
    let selectedAudioOutput = null;
    if (!isRobot) {
      selectedVideoSource = localStorage.getItem("selectedVideoSource");
      selectedAudioSource = localStorage.getItem("selectedAudioSource");
      selectedAudioOutput = localStorage.getItem("selectedAudioOutput");
    }
    this.state = {
      isRobot,
      selectedVideoSource,
      selectedAudioSource,
      selectedAudioOutput,

      selectedRobotVideoSource: null,
      selectedRobotAudioSource: null,
      selectedRobotAudioOutput: null,

      hasSentIAmMsg: false,
      localVideoSteam: null,
      remoteVideoStream: null,

      connectedToServer: false,

      mediaDevices: [],
      robotMediaDevices: [],

      // This is set to true if the robot has told the signalling server that it is online.
      robotConnected: false,

      // This is set to true if the controller has told the signalling server that it is online.
      // The controllerConnected is only used when this webapp is pretending to be a robot.
      controllerConnected: false,

      iceConnectionState: null,
      signalingState: null,
      peerConnection: null,
    };

    if (isRobot) {
      this.state.steering = 0;
      this.state.throttle = 0;
    }

    const socket = io.connect(null,
      {
        reconnection: true,
        reconnectionDelay: 1000,
        reconnectionDelayMax : 5000,
        reconnectionAttempts: 99999,
      }
    );
    this.socket = socket;

    socket.on('connect', () => {
      console.log("Connected to the server");

      this.setState({connectedToServer: true});
      if (this.state.localVideoSteam) {
        this.sendIAmMessage();
      }
    });

    socket.on('disconnect', () => {
      console.log("Lost the connection to the server");
      this.removePeerConnectionIfNeeded();
      this.setState({
        connectedToServer: false,
        });
    });
    socket.on('robot-connected', () => {
      this.setState({'robotConnected': true});
      console.log("Got a 'robot-connected', message, so I'm creating a RTCPeerConnection() and sending an offer now.");
      this.createPeerConnection();
      this.createOffer();
    });

    socket.on('robot-disconnected', () => {
      console.log("Got a 'robot-disconnected' message.");
      this.setState({'robotConnected': false});
      this.removePeerConnectionIfNeeded();
    });

    socket.on('controller-connected', () => {
      console.log("Got a 'controller-connected' message.");
      this.setState({'controllerConnected': true});
    });

    socket.on('controller-disconnected', () => {
      console.log("Got a 'controller-disconnected' message.");
      this.setState({'controllerConnected': false});
      this.removePeerConnectionIfNeeded();
    });


    socket.on('webrtc-offer', (sessionDescription) => {
      console.log("Got a 'webrtc-offer' message.");

      let pc = this.state.peerConnection;
      if (!pc) {
        pc = this.createPeerConnection();
      }
      pc.setRemoteDescription(sessionDescription).then((test) => {
        //console.log("Successfully called peerConnection.setRemoteDescription() for the offer from the controller.");
        pc.createAnswer().then(
          (sessionDescription) => {
            pc.setLocalDescription(sessionDescription);
            console.log('sending webrtc-answer');
            this.socket.emit("webrtc-answer", sessionDescription);
            pc.onnegotiationneeded = this.handleNegotiationNeeded;
          },
          (error) => {
            console.log('Failed to create answer session description: ' + error.toString());
          }
        );
      }).catch((error) => {
        console.log('Failed to call peerConnection.setRemoteDescription(): ' + error.toString());
      });
    });


    socket.on('webrtc-answer', (sessionDescription) => {
      console.log("Got a 'webrtc-answer' message.");
      this.state.peerConnection.setRemoteDescription(sessionDescription).then((test) => {
        //console.log("Successfully called peerConnection.setRemoteDescription() for the answer from the robot.");
        this.state.peerConnection.onnegotiationneeded = this.handleNegotiationNeeded;

      }).catch((error) => {
        console.log('Failed to call peerConnection.setRemoteDescription(): ' + error.toString());
      });
    });


    this.socket.on("ice-candidate", (message) => {
      if (!this.state.peerConnection) {
        console.log("Received an 'ice-candidate' message, but this.state.peerConnection is null.");
        return;
      }

      const candidate = new RTCIceCandidate({
        sdpMLineIndex: message.sdpMLineIndex,
        candidate: message.candidate
      });
      this.state.peerConnection.addIceCandidate(candidate).then(() => {
        //console.log("Successfully added a remote ice-canditate.");
      }).catch((error) => {
        console.error("Failed to add the remote ice-canditate:" + error.toString(), error);
      });

    });


    navigator.mediaDevices.enumerateDevices().then(this.gotDevices).catch(handleEnumerateDevicesError);
  }

  sendRobotDeviceInfosMessageIfNeeded = () => {
    if (this.state.isRobot && this.state.dataChannel && this.state.dataChannelIsOpen && this.state.mediaDevices) {
      console.log("sendRobotDeviceInfosMessageIfNeeded() sending message.");
      this.state.dataChannel.send(JSON.stringify({
        "type": "robot-mediadevices",
        "mediadevices": this.state.mediaDevices,
        "selectedVideoSource": this.state.selectedVideoSource,
        "selectedAudioSource": this.state.selectedAudioSource,
        "selectedAudioOutput": this.state.selectedAudioOutput,
      }));
    } else {
      console.log("sendRobotDeviceInfosMessageIfNeeded() running, but not sending message.");
    }
  }

  sendIAmMessage = () => {
    this.setState({hasSentIAmMsg: true});
    if (this.state.isRobot) {
      console.log("Sending the 'i-am-the-robot' message.");
      this.socket.emit('i-am-the-robot');
    } else {
      console.log("Sending the 'i-am-the-controller' message.");
      this.socket.emit('i-am-the-controller');
    }
  }

  createPeerConnection = () => {
    const thisthis = this;
    this.removePeerConnectionIfNeeded();
    try {
      const pcConfig = {
        'iceServers': [{
          'urls': 'stun:stun.l.google.com:19302'
        }]
      };

      const pc = new RTCPeerConnection(pcConfig);

      pc.onicecandidate = this.handleIceCandidate;
      pc.ontrack = this.handleRemoteTrackAdded;
      pc.oniceconnectionstatechange = this.handleICEConnectionStateChange;
      pc.onsignalingstatechange = this.handleSignalingStateChange;


      const newState = {
        peerConnection: pc,
        iceConnectionState: pc.iceConnectionState,
        signalingState: pc.signalingState,
      }


      if (!this.state.isRobot) {
        const dataChannelOptions = {
          ordered: true,
          maxRetransmitTime: 3000, // in milliseconds
        };
        console.log("Creating a datachannel");
        const dataChannel = pc.createDataChannel("robotcontrol", dataChannelOptions);

        this.setState({
          dataChannel,
          dataChannelIsOpen: false,
        });

        dataChannel.onerror = function (error) {
          console.log("Data Channel Error:", error);
        };

        dataChannel.onmessage = function (event) {
          let message = event.data;
          console.log("Got Data Channel Message:", message);
          message = JSON.parse(message);
          if (message.type === "robot-mediadevices") {
            console.info("Got a robot-mediadevices message: " + JSON.stringify(message.mediadevices));
            thisthis.setState({
              robotMediaDevices: message.mediadevices,
              selectedRobotVideoSource: message.selectedVideoSource,
              selectedRobotAudioSource: message.selectedAudioSource,
              selectedRobotAudioOutput: message.selectedAudioOutput,
            });
          } else {
            console.error("Got an unknown dataChannel message:" + JSON.stringify(message));
          }
        };

        dataChannel.onopen = function () {
          console.log("Got a dataChannel 'onopen' event.");
          thisthis.setState({dataChannelIsOpen: true});
        };

        dataChannel.onclose = function () {
          console.log("Got a dataChannel 'onclose' event.");
          thisthis.setState({dataChannelIsOpen: false});
        };
      }
      pc.ondatachannel = this.onDataChannel;

      this.setState(newState);
      console.log('Created RTCPeerConnnection');
      const localVideoSteam = this.state.localVideoSteam;
      if (localVideoSteam) {
        localVideoSteam.getTracks().forEach(function(track) {
          console.log("Adding local track to the peerConnection: " + track.toString());
          pc.addTrack(track, localVideoSteam);
        });

        //console.log("Adding local stream to the peerConnection: " + localVideoSteam.toString());
        //pc.addStream(localVideoSteam);
      }

      return pc;
    } catch (e) {
      console.log('Failed to create PeerConnection, exception: ' + e.message);
      alert('Cannot create RTCPeerConnection object.');
      return;
    }
  }

  handleNegotiationNeeded = () => {
    const signalingState = this.state.signalingState;
    console.log("handleNegotiationNeeded() running. signalingState:" + signalingState);

    console.log("TODO: get re-negotiation to work properly. For now we just disconnect and reconnect from scratch.")
    //this.createOffer();
    this.removePeerConnectionIfNeeded();
    this.socket.disconnect();
    this.socket.open();
  }


  /*
   * This is called on the robot when the controller has created a datachannel.
   */
  onDataChannel = (event) => {
    const thisthis = this;
    const dataChannel = event.channel;
    console.log("onDataChannel() running. dataChannel:" + dataChannel);

    if (!this.state.isRobot) {
      console.log("onDataChannel() running, but I am not a robot!");
      return;
    }

    this.setState({
      dataChannel,
      dataChannelIsOpen: true,
    });

    dataChannel.onerror = function (error) {
      console.log("Data Channel Error:", error);
    };

    dataChannel.onmessage = function (event) {
      let message = event.data;
      console.log("Got Data Channel Message:", message);
      message = JSON.parse(message);
      if (message.type === "selected-video-source") {
        console.info("Got a selected-video-source message: " + JSON.stringify(message.deviceId));
        thisthis.setState({selectedVideoSource: message.deviceId});
      } else if (message.type === "movement-control") {
        console.info("Got a movement-control message: " + JSON.stringify(message));
        thisthis.setState({
          steering: message.steering,
          throttle: message.throttle,
          });
      } else {

        console.error("Got an unknown dataChannel message:" + JSON.stringify(message));
      }

    };

    dataChannel.onopen = function () {
      console.log("Got a dataChannel 'onopen' event.");
      thisthis.setState({
        dataChannelIsOpen: true,
      }, thisthis.sendRobotDeviceInfosMessageIfNeeded);
    };

    dataChannel.onclose = function () {
      console.log("Got a dataChannel 'onclose' event.");
      thisthis.setState({
        dataChannelIsOpen: false,
      });
    };
  }


  createOffer = () => {
    console.log("App.createOffer() running");
    this.state.peerConnection.createOffer(
      (sessionDescription) => {
        this.state.peerConnection.setLocalDescription(sessionDescription);
        //console.log('handleCreateOfferSuccess sending message', sessionDescription);
        this.socket.emit("webrtc-offer", sessionDescription);
      },
      (error) => {
        console.log('Failed to create session description: ' + error.toString());
      }
    );
  }


  /* This is called when a new remote media track has been added.*/
  handleRemoteTrackAdded = (event) => {
    const remoteVideoStream = event.streams[0];
    console.log('handleRemoteTrackAdded() running. stream: ' + remoteVideoStream);
    remoteVideoStream.onremovetrack = this.handleRemoteTrackRemoved;
    this.setState({remoteVideoStream});
  }

  handleSignalingStateChange = (event) => {
    let newstate = null;
    if (this.state.peerConnection) {
      newstate = this.state.peerConnection.signalingState;
    }
    console.log("handleSignalingStateChange(): " + this.state.signalingState + " => " + newstate);
    this.setState({signalingState: newstate});
  }

  /* This is called when the RTCPeerConnection state has changed.*/
  handleICEConnectionStateChange= (event) => {
    let newstate = null;
    if (this.state.peerConnection) {
      newstate = this.state.peerConnection.iceConnectionState;
    }
    console.log("handleICEConnectionStateChange(): " + this.state.iceConnectionState + " => " + newstate);
    this.setState({iceConnectionState: newstate});
  }

  /* This is called when the PeerConnection has generated a local ice candidate. */
  handleIceCandidate = (event) => {
    //console.log('icecandidate event: ', event);
    if (event.candidate) {
      this.socket.emit("ice-candidate", {
        sdpMLineIndex: event.candidate.sdpMLineIndex,
        sdpMid: event.candidate.sdpMid,
        candidate: event.candidate.candidate
      });
    } else {
      console.log('End of candidates.');
    }
  }


  removePeerConnectionIfNeeded = () => {
    console.log("removePeerConnectionIfNeeded () running.")
    if (this.state.peerConnection) {
      this.state.peerConnection.close();
      console.log("Closed the RTCPeerConnection.");
    }
    const newState = {
      peerConnection: null,
      iceConnectionState: null,
      hasSentIAmMsg: false,
      remoteVideoStream: null,
      robotMediaDevices: [],
    };

    this.setState(newState);
  }


  changeLocalAudioOrVideoSourceIfNeeded = () => {
    if ((this.state.assignedVideoSource !== this.state.selectedVideoSource)
    || (this.state.assignedAudioSource !== this.state.selectedAudioSource)) {

      const localVideoSteam = this.state.localVideoSteam;
      const isCanvasStream = localVideoSteam === this.localVideoStreamFromCanvas;
      if (localVideoSteam) {
        const peerConnection = this.state.peerConnection;
        localVideoSteam.getTracks().forEach(function(trackToRemove) {
          if (peerConnection) {
            peerConnection.getSenders().forEach(function(sender) {
              if (sender.track === trackToRemove) {
                peerConnection.removeTrack(sender);
              }
            });
          }
          if (!isCanvasStream) {
            trackToRemove.stop();
          }
        });
        this.setState({localVideoSteam: null,
        });
      }
      const audioSource = this.state.selectedAudioSource;
      const videoSource = this.state.selectedVideoSource;
      if (videoSource || audioSource) {
        const constraints = {}

        if (videoSource) {
          constraints.video = {deviceId: {exact: videoSource}};
        }

        if (audioSource) {
           constraints.audio = {deviceId: {exact: audioSource}};
        }
        console.log("changeLocalAudioOrVideoSourceIfNeeded() using constraints:" + JSON.stringify(constraints));
        navigator.mediaDevices.getUserMedia(constraints).then(this.gotStream).then(this.gotDevices).catch(
          handleGetUserMediaError);
      } else {
        this.setState({assignedVideoSource: "",
                       assignedAudioSource: "",
                       localVideoSteam: null,
                       });
        this.gotStream(this.localVideoStreamFromCanvas);
      }
    }
  }

  componentDidMount = () => {
    console.log("App.componentDidMount()")
    const canvas = this.localVideoCanvasElement;

    // we must wait for the dom to be rendered, otherwise the call to canvas.captureStream() wont work.
    setTimeout(() => {
      const ctx = canvas.getContext("2d");
      this.localVideoStreamFromCanvas = canvas.captureStream(25);
      console.log("this.localVideoStreamFromCanvas:" + this.localVideoStreamFromCanvas.toString());
      this.changeLocalAudioOrVideoSourceIfNeeded();
      let canvasBackgroundColour;
      if (this.state.isRobot) {
        canvasBackgroundColour = "#BFBFBF";
      } else {
        canvasBackgroundColour = "#5F5F5F";
      }

      setInterval(() => {
          ctx.font = "150px Arial";
          const d = new Date();
          let n = (d.getTime() / 1000).toFixed(0);

          ctx.fillStyle =canvasBackgroundColour;
          ctx.fillRect(0, 0, canvas.width, canvas.height);

          ctx.fillStyle ="#000000";

          if (this.state.isRobot) {
            ctx.fillText("Robot time:", 5, 150);
            ctx.fillText(n, 5, 300);
            ctx.fillText("steering:" + this.state.steering, 30, 500);
            ctx.fillText("throttle:" + this.state.throttle, 30, 700);
          } else {
            ctx.fillText("Controller time:", 5, 150);
            ctx.fillText(n, 5, 300);
          }


        }, 100
      )
    }, 100);

    setInterval(this.logStats, 3000);

  }

  componentDidUpdate = (prevProps, prevState, snapshot) => {
    console.log("componentDidUpdate() running.")
    if (prevState.selectedVideoSource !== this.state.selectedVideoSource
    || prevState.selectedAudioSource !== this.state.selectedAudioSource) {
      console.log("Updating local video and/or audio inputs.")
      this.changeLocalAudioOrVideoSourceIfNeeded();
      return;
    }

    if (prevState.selectedRobotVideoSource !== this.state.selectedRobotVideoSource) {
      console.log("Updating robot video source.");
      if (this.state.dataChannel) {
        this.state.dataChannel.send(JSON.stringify({
          "type": "selected-video-source",
          "deviceId": this.state.selectedRobotVideoSource,
        }));
      }
    }

    if (prevState.selectedRobotAudioSource !== this.state.selectedRobotAudioSource) {
      console.log("Updating robot audio source.");
      if (this.state.dataChannel) {
        this.state.dataChannel.send(JSON.stringify({
          "type": "selected-audio-source",
          "deviceId": this.state.selectedRobotAudioSource,
        }));
      }
    }

    if (prevState.selectedRobotAudioOutput !== this.state.selectedRobotAudioOutput) {
      console.log("Updating robot audio output.");
      if (this.state.dataChannel) {
        this.state.dataChannel.send(JSON.stringify({
          "type": "selected-audio-output",
          "deviceId": this.state.selectedRobotAudioOutput,
        }));
      }
    }

  }

  setMainState = (updatedState) => {
    console.log("setMainState() running. updatedState: " + JSON.stringify(updatedState));
    this.setState(updatedState);
  }

  gotDevices = (deviceInfos) => {
    console.log("gotDevices() deviceInfos: " + JSON.stringify(deviceInfos));
    this.setState({mediaDevices: deviceInfos}, this.sendRobotDeviceInfosMessageIfNeeded);
  }

  gotStream = (stream) => {
    console.log("gotStream() stream:" + stream);
    const localVideoSteam = stream;
    this.setState({assignedVideoSource: this.state.selectedVideoSource,
                   assignedAudioSource: this.state.selectedAudioSource,
                   localVideoSteam,
                   });

    if (localVideoSteam) {
      const peerConnection = this.state.peerConnection;
      if (peerConnection) {
        localVideoSteam.getTracks().forEach(function(track) {
          console.log("Adding local track to the peerConnection: " + track.toString());
          peerConnection.addTrack(track, localVideoSteam);
        });
        //console.log("Adding local stream to the peerConnection: " + localVideoSteam.toString());
        //peerConnection.addStream(localVideoSteam);
      } else {
        if (!this.state.hasSentIAmMsg) {
          console.log("Got a videostream, so I'm sending the 'i-am-<robot|controller>' message now.");
          this.sendIAmMessage();
        }
      }
    }

    // Refresh button list in case labels have become available
    return navigator.mediaDevices.enumerateDevices();
  }


  logStats = () => {
  /*
    console.log("logStats() running");

    if (this.state.peerConnection) {
        const currentRemoteDescription = this.state.peerConnection.currentRemoteDescription;
        console.log("currentRemoteDescription: ", currentRemoteDescription);

      this.state.peerConnection.getStats(null)
            .then(function(stats) {
                console.log("logStats() got stats:", stats);

                stats.forEach(function(value, key) {
                    console.log("logStats() stats[" + key + "]:", value);
                });

              Object.keys(stats).forEach(function(key) {
                console.log("logStats() stats[" + key + "]:", stats[key]);

                if (stats[key].type === "candidatepair" &&
                    stats[key].nominated && stats[key].state === "succeeded") {
                  var remote = stats[stats[key].remoteCandidateId];
                  console.log("Connected to: " + remote.ipAddress +":"+
                              remote.portNumber +" "+ remote.transport +
                              " "+ remote.candidateType);
                }
              });
            })
      .catch(function(e) { console.log(e.name); });
    }*/
  }


/*
            <div className="nav">
              <ul>
                <li>
                  <NavLink exact to={this.state.isRobot?'/?isRobot=true':'/'}>{this.state.isRobot ? 'RobotDisplay' : 'Cockpit'}</NavLink>
                </li>
                <li>
                  <NavLink to={this.state.isRobot?'/config?isRobot=true':'/config'}>Configuration</NavLink>
                </li>
              </ul>
            </div>


            <Route path="/config" render={()=><Configuration
                                                 mediaDevices={this.state.mediaDevices}
                                                 robotMediaDevices={this.state.robotMediaDevices}
                                                 selectedVideoSource={this.state.selectedVideoSource}
                                                 selectedAudioSource={this.state.selectedAudioSource}
                                                 selectedAudioOutput={this.state.selectedAudioOutput}
                                                 selectedRobotVideoSource={this.state.selectedRobotVideoSource}
                                                 selectedRobotAudioSource={this.state.selectedRobotAudioSource}
                                                 selectedRobotAudioOutput={this.state.selectedRobotAudioOutput}
                                                 isRobot={this.state.isRobot}
                                                 iceConnectionState={this.state.iceConnectionState}
                                                 setMainState={this.setMainState}
                                                 />}

                                                />

*/

  render() {
    const localVideoPortraitMode = false; //this.state.isRobot
    return (
      <Router>
        <div className={this.state.isRobot?'is-robot':'is-controller'}>
            <Route exact path="/"
              render={()=> {
                if(this.state.isRobot) {
                  return (<RobotDisplay
                  localVideoSteam={this.state.localVideoSteam}
                  remoteVideoStream={this.state.remoteVideoStream}
                  connectedToServer={this.state.connectedToServer}

                  selectedVideoSource={this.state.selectedVideoSource}
                  selectedAudioSource={this.state.selectedAudioSource}
                  selectedAudioOutput={this.state.selectedAudioOutput}

                  isRobot={this.state.isRobot}

                  robotConnected={this.state.robotConnected}
                  controllerConnected={this.state.controllerConnected}
                  iceConnectionState={this.state.iceConnectionState}
                  signalingState={this.state.signalingState}

                  dataChannel={this.state.dataChannel}
                  dataChannelIsOpen={this.state.dataChannelIsOpen}

                  steering={this.state.steering}
                  throttle={this.state.throttle}
                  />);
                } else {
                  return (<Cockpit
                  setMainState={this.setMainState}
                  localVideoSteam={this.state.localVideoSteam}
                  remoteVideoStream={this.state.remoteVideoStream}
                  connectedToServer={this.state.connectedToServer}

                  mediaDevices={this.state.mediaDevices}
                  selectedVideoSource={this.state.selectedVideoSource}
                  selectedAudioSource={this.state.selectedAudioSource}
                  selectedAudioOutput={this.state.selectedAudioOutput}

                  robotMediaDevices={this.state.robotMediaDevices}
                  selectedRobotVideoSource={this.state.selectedRobotVideoSource}
                  selectedRobotAudioSource={this.state.selectedRobotAudioSource}
                  selectedRobotAudioOutput={this.state.selectedRobotAudioOutput}

                  isRobot={this.state.isRobot}

                  robotConnected={this.state.robotConnected}
                  controllerConnected={this.state.controllerConnected}
                  iceConnectionState={this.state.iceConnectionState}
                  signalingState={this.state.signalingState}

                  dataChannel={this.state.dataChannel}
                  dataChannelIsOpen={this.state.dataChannelIsOpen}
                  />);
                }
              } } />


          <canvas height={localVideoPortraitMode ? 1920 : 1080} width={localVideoPortraitMode ? 1080: 1920 }
              className="local-video-canvas" ref={(canvas) => { this.localVideoCanvasElement = canvas; }} />
        </div>
      </Router>
    );
  }
}

export default App;

import React, { Component } from 'react';
import { BrowserRouter as Router, Route, NavLink} from "react-router-dom";
import io from 'socket.io-client';
import './App.css';
import { Configuration} from './Configuration.js';
import { Cockpit } from './Cockpit.js';


function handleError(error) {
  console.log('navigator.getUserMedia error: ', error);
  alert('navigator.getUserMedia error: ' + error);
}

class App extends Component {
  constructor(props) {
    super(props);
    this.state = {
      selectedVideoSource: localStorage.getItem("selectedVideoSource"),
      selectedAudioSource: localStorage.getItem("selectedAudioSource"),
      selectedAudioOutput: localStorage.getItem("selectedAudioOutput"),
      simulateRobot: localStorage.getItem("simulateRobot") === "true",
      hasSentIAmMsg: false,
      localVideoSteam: null,
      remoteVideoStream: null,

      connectedToServer: false,

      mediaDevices: [],

      // This is set to true if the robot has told the signalling server that it is online.
      robotConnected: false,

      // This is set to true if the controller has told the signalling server that it is online.
      // The controllerConnected is only used when this webapp is pretending to be a robot.
      controllerConnected: false,

      iceConnectionState: null,
      signalingState: null,
      peerConnection: null,

    };

    const socket = io.connect();
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

    // The controller* messages are only used when this webapp is pretending to be a robot.
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
        console.log('Failed to call peerPonnection.setRemoteDescription(): ' + error.toString());
      });
    });


    socket.on('webrtc-answer', (sessionDescription) => {
      console.log("Got a 'webrtc-answer' message.");
      this.state.peerConnection.setRemoteDescription(sessionDescription).then((test) => {
        //console.log("Successfully called peerConnection.setRemoteDescription() for the answer from the robot.");
        this.state.peerConnection.onnegotiationneeded = this.handleNegotiationNeeded;

      }).catch((error) => {
        console.log('Failed to call peerPonnection.setRemoteDescription(): ' + error.toString());
      });
    });


    this.socket.on("ice-candidate", (message) => {
      //console.log("Received an 'ice-candidate' message");
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



    navigator.mediaDevices.enumerateDevices().then(this.gotDevices).catch(handleError);
  }

  sendIAmMessage = () => {
    this.setState({hasSentIAmMsg: true});
    if (this.state.simulateRobot) {
      console.log("Sending the 'i-am-the-robot' message.");
      this.socket.emit('i-am-the-robot');
    } else {
      console.log("Sending the 'i-am-the-controller' message.");
      this.socket.emit('i-am-the-controller');
    }
  }

  createPeerConnection = () => {
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
      pc.onremovestream = this.handleRemoteStreamRemoved;
      pc.oniceconnectionstatechange = this.handleICEConnectionStateChange;
      pc.onsignalingstatechange = this.handleSignalingStateChange;
      this.setState({
        peerConnection: pc,
        iceConnectionState: pc.iceConnectionState,
        signalingState: pc.signalingState,
      });
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


  createOffer = () => {
    console.log("App.createOffeR() running");
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
    if (this.state.peerConnection) {
      this.state.peerConnection.close();
      console.log("Closed the RTCPeerConnection.");
    }
    this.setState({
      peerConnection: null,
      iceConnectionState: null,
      hasSentIAmMsg: false,
      });
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
        navigator.mediaDevices.getUserMedia(constraints).then(this.gotStream).then(this.gotDevices).catch(handleError);
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
      setInterval(() => {
          ctx.font = "25px Arial";
          const d = new Date();
          let n = d.toISOString();

          ctx.fillStyle ="#FFFFFF";
          ctx.fillRect(0, 0, canvas.width, canvas.height);
          ctx.fillStyle ="#00FFFF";

          if (this.state.simulateRobot) {
            ctx.fillText("Robot time:", 5, 30);
          } else {
            ctx.fillText("Controller time:",5,30);
          }

          ctx.fillText(n, 5, 60);

        }, 1000
      )
    }, 100);

  }

  componentDidUpdate = (prevProps, prevState, snapshot) => {
    if (prevState.selectedVideoSource !== this.state.selectedVideoSource
    || prevState.selectedAudioSource !== this.state.selectedAudioSource) {
      console.log("Updating local video and/or audio inputs.")
      this.changeLocalAudioOrVideoSourceIfNeeded();
      return;
    }
  }

  setMainState = (updatedState) => {
    this.setState(updatedState);
  }

  gotDevices = (deviceInfos) => {
    this.setState({mediaDevices: deviceInfos});
  }

  gotStream = (stream) => {
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

  render() {
    return (
      <Router>
        <div>
          <div>
            <div className="nav">
              <ul>
                <li>
                  <NavLink exact to="/">Cockpit</NavLink>
                </li>
                <li>
                  <NavLink to="/config">Configuration</NavLink>
                </li>
              </ul>
            </div>

            <hr />
            <Route exact path="/" render={()=> <Cockpit
              localVideoSteam={this.state.localVideoSteam}
              remoteVideoStream={this.state.remoteVideoStream}
              connectedToServer={this.state.connectedToServer}


              selectedVideoSource={this.state.selectedVideoSource}
              selectedAudioSource={this.state.selectedAudioSource}
              selectedAudioOutput={this.state.selectedAudioOutput}

              simulateRobot={this.state.simulateRobot}

              robotConnected={this.state.robotConnected}
              controllerConnected={this.state.controllerConnected}
              iceConnectionState={this.state.iceConnectionState}
              signalingState={this.state.signalingState}

              />
            } />
            <Route path="/config" render={()=><Configuration
                                                 mediaDevices={this.state.mediaDevices}
                                                 selectedVideoSource={this.state.selectedVideoSource}
                                                 selectedAudioSource={this.state.selectedAudioSource}
                                                 selectedAudioOutput={this.state.selectedAudioOutput}
                                                 simulateRobot={this.state.simulateRobot}
                                                 iceConnectionState={this.state.iceConnectionState}
                                                 setMainState={this.setMainState}
                                                 />}

                                                />

          </div>
          <div>
            <h3>Canvas that is used when no camera is selected:</h3>
            <canvas style={{borderStyle:"solid"}} className="local-video-canvas" ref={(canvas) => { this.localVideoCanvasElement = canvas; }} />

          </div>
        </div>
      </Router>
    );
  }
}

export default App;

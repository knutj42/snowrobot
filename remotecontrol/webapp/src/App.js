import React, { Component } from 'react';
import { BrowserRouter as Router, Route, Link } from "react-router-dom";
import io from 'socket.io-client';
import './App.css';
import { Configuration} from './Configuration.js';

var socket = io.connect();

class Home extends Component {
  render() {
    return (
      <div>Home component</div>
      );
  }
}


function handleError(error) {
  console.log('navigator.getUserMedia error: ', error);
  alert('navigator.getUserMedia error: ' + error);
}


function sendMessage(message) {
  console.log('Client sending message: ', message);
  socket.emit('message', message);
}

class App extends Component {
  constructor(props) {
    super(props);
    this.videoElement = null;
    this.state = {
      selectedVideoSource: localStorage.getItem("selectedVideoSource"),
      selectedAudioSource: localStorage.getItem("selectedAudioSource"),
      selectedAudioOutput: localStorage.getItem("selectedAudioOutput"),
      mediaDevices: [],
      controllerConnected: false,
      robotConnected: false,
    };

    socket.on('robot-connected', () => {this.setState({'robotConnected': true})});
    socket.on('robot-disconnected', () => {this.setState({'robotConnected': false})});

    navigator.mediaDevices.enumerateDevices().then(this.gotDevices).catch(handleError);
  }

  createPeerConnection = () => {
    this.removePeerConnectionIfNeeded();
    try {
      const pc = new RTCPeerConnection(null);
      pc.onicecandidate = this.handleIceCandidate;
      pc.onaddstream = this.handleRemoteStreamAdded;
      pc.onremovestream = this.handleRemoteStreamRemoved;
      this.setState({peerConnection: pc});
      console.log('Created RTCPeerConnnection');

    } catch (e) {
      console.log('Failed to create PeerConnection, exception: ' + e.message);
      alert('Cannot create RTCPeerConnection object.');
      return;
    }
  }

  /* This is called when the PeerConnection has generated a local ice candidate. */
  handleIceCandidate = (event) => {
    console.log('icecandidate event: ', event);
    if (event.candidate) {
      sendMessage({
        type: 'candidate',
        label: event.candidate.sdpMLineIndex,
        id: event.candidate.sdpMid,
        candidate: event.candidate.candidate
      });
    } else {
      console.log('End of candidates.');
    }
  }


  removePeerConnectionIfNeeded = () => {
    if (this.state.peerConnection) {
      this.state.peerConnection.close();
    }
    this.setState({peerConnection: null});
  }


  changeAudioOrVideoSourceIfNeeded = () => {
    if ((this.state.assignedVideoSource !== this.state.selectedVideoSource)
    || (this.state.assignedAudioSource !== this.state.selectedAudioSource)) {
      // Something has changed, so clear current streams and connections.
      this.removePeerConnectionIfNeeded();

      if (this.state.localVideoSteam) {
        this.state.localVideoSteam.getTracks().forEach(function(track) {
          track.stop();
        });
        this.setState({localVideoSteam: null,
        });
        this.videoElement.srcObject = null;
      }
      const audioSource = this.state.selectedAudioSource;
      const videoSource = this.state.selectedVideoSource;
      if (videoSource) {
        const constraints = {
          video: {deviceId: {exact: videoSource}}
        };

        if (audioSource) {
           constraints.audio = {deviceId: {exact: audioSource}};
        }
        navigator.mediaDevices.getUserMedia(constraints).then(this.gotStream).then(this.gotDevices).catch(handleError);
      } else {
        this.setState({assignedVideoSource: "",
                       assignedAudioSource: "",
                       localVideoSteam: null,
                       });
      }
    }
  }

  componentDidMount = () => {
    this.changeAudioOrVideoSourceIfNeeded();
  }

  componentDidUpdate = (prevProps, prevState, snapshot) => {
    if (prevState.selectedVideoSource !== this.state.selectedVideoSource
    || prevState.selectedAudioSource !== this.state.selectedAudioSource) {
      this.changeAudioOrVideoSourceIfNeeded();
    }
  }

  setMainState = (updatedState) => {
    this.setState(updatedState);
  }

  gotDevices = (deviceInfos) => {
    this.setState({mediaDevices: deviceInfos});
  }

  gotStream = (stream) => {
    this.videoElement.srcObject = stream;

    this.setState({assignedVideoSource: this.state.selectedVideoSource,
                   assignedAudioSource: this.state.selectedAudioSource,
                   localVideoSteam: stream,
                   });

    // Refresh button list in case labels have become available
    return navigator.mediaDevices.enumerateDevices();
  }

  render() {
    let configIsRequiredMsg = "";
    if (!this.state.selectedVideoSource) {
      configIsRequiredMsg = "You must select which camera to use.";
    }
    return (
      <Router>
        <div>
          <h1>Control Center</h1>
          <div>
            <ul>
              <li className={this.state.selectedVideoSource ? "ok" : "pending"}>Video source</li>
            </ul>
          </div>

          <div>
            <ul>
              <li>
                <Link to="/">Home</Link>
              </li>
              <li>
                <Link to="/config">Configuration</Link>
              </li>
            </ul>

            <div>
              <video autoPlay ref={(video) => { this.videoElement = video; }} />
            </div>

            <hr />
            {!configIsRequiredMsg &&
              <div>
                <Route exact path="/" component={Home} />
                <Route path="/config" render={()=><Configuration
                                                     mediaDevices={this.state.mediaDevices}
                                                     selectedVideoSource={this.state.selectedVideoSource}
                                                     selectedAudioSource={this.state.selectedAudioSource}
                                                     selectedAudioOutput={this.state.selectedAudioOutput}
                                                     setMainState={this.setMainState}
                                                     />}

                                                    />
              </div>
            }

            {configIsRequiredMsg &&
              <div>
                <div className="configIsRequiredMsg">{configIsRequiredMsg}</div>
                <Configuration
                   mediaDevices={this.state.mediaDevices}
                   selectedVideoSource={this.state.selectedVideoSource}
                   selectedAudioSource={this.state.selectedAudioSource}
                   selectedAudioOutput={this.state.selectedAudioOutput}
                   setMainState={this.setMainState}
                   />
              </div>
            }

          </div>
        </div>
      </Router>
    );
  }
}

export default App;

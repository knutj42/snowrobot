import React, { Component } from 'react';

import './RobotDisplay.css';


export class RobotDisplay extends Component {
  constructor(props) {
    super(props);
    this.localVideoElement = null;
    this.remoteVideoElement = null;
    this.state = {
    };
  }

  render() {

    const hasConnectionToServer = this.props.connectedToServer;
    const robotConnected = this.props.robotConnected;
    const controllerConnected = this.props.controllerConnected;
    const iceConnectionState = this.props.iceConnectionState;
    const signalingState = this.props.signalingState;
    const okClass = "ok";
    const errorClass = "error";
    let hasErrors = false;

    const statusListElements = [
              (<li key="server" className={hasConnectionToServer?okClass:errorClass} >Server</li>),

              ];
    if (this.props.isRobot) {
      if (!controllerConnected) {
        hasErrors = true;
      }
      const peerStatus = controllerConnected ? "online" : "offline";
      statusListElements.push(<li key="controller" className={controllerConnected?okClass:errorClass}  >Controller is {peerStatus}</li>);
    } else {
      if (!robotConnected) {
        hasErrors = true;
      }
      const peerStatus = robotConnected ? "online" : "offline";
      statusListElements.push(<li key="robot" className={robotConnected?okClass:errorClass}  >Robot is {peerStatus}</li>);
    }

    const iceIsConnected = iceConnectionState==="connected" || iceConnectionState==="completed";
    if (!iceIsConnected) {
      hasErrors = true;
    }

    statusListElements.push(<li key="iceConnectionState" className={iceIsConnected?okClass:errorClass} >iceConnectionState: {iceConnectionState || "none"}</li>);

    const signalingStateIsOk = signalingState==="stable";
    if (!signalingStateIsOk) {
      hasErrors = true;
    }
    statusListElements.push(<li key="signalingState" className={signalingStateIsOk?okClass:errorClass} >signalingState: {signalingState || "none"}</li>);

    let statusClassName = "status";
    if (hasErrors) {
      statusClassName += " has-errors";
    } else {
      statusClassName += " no-errors";
    }

    return (
      <div className="robot-display">
        <video className="remote-video"  autoPlay ref={(video) => { this.remoteVideoElement = video; }} />
        <video className="local-video" autoPlay ref={(video) => { this.localVideoElement = video; }} />

        <div className={statusClassName}>
          <h3>Status:</h3>
          <ul>
            {statusListElements}
          </ul>
        </div>

      </div>
      );
  }

  componentDidMount = () => {
    if (this.props.localVideoSteam) {
      console.log("Setting this.localVideoElement.srcObject to " + this.props.localVideoSteam);
      this.localVideoElement.srcObject = this.props.localVideoSteam;
    }

    if (this.props.remoteVideoStream) {
      console.log("Setting this.remoteVideoElement.srcObject to " + this.props.remoteVideoStream);
      this.remoteVideoElement.srcObject = this.props.remoteVideoStream;
    }

  }

  componentDidUpdate = (prevProps, prevState, snapshot) => {
    if (prevProps.localVideoSteam !== this.props.localVideoSteam) {
      console.log("Setting this.localVideoElement.srcObject to " + this.props.localVideoSteam);
      this.localVideoElement.srcObject = this.props.localVideoSteam;
    }

    if (prevProps.remoteVideoStream !== this.props.remoteVideoStream) {
      console.log("Setting this.remoteVideoElement.srcObject to " + this.props.remoteVideoStream);
      this.remoteVideoElement.srcObject = this.props.remoteVideoStream;
    }
  }

}

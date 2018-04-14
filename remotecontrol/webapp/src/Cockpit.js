import React, { Component } from 'react';

import './Cockpit.css';


export class Cockpit extends Component {
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
    const hasCamera = this.props.localVideoSteam;
    const iceConnectionState = this.props.iceConnectionState;
    const okClass = "ok";
    const errorClass = "error";

    const statusListElements = [
              (<li key="camera" className={hasCamera?okClass:errorClass}             >Camera</li>),
              (<li key="server" className={hasConnectionToServer?okClass:errorClass} >Server</li>),

              ];
    if (this.props.simulateRobot) {
      statusListElements.push(<li key="controller" className={controllerConnected?okClass:errorClass}  >Controller is online</li>);
    } else {
      statusListElements.push(<li key="robot" className={robotConnected?okClass:errorClass}  >Robot is online</li>);
    }

    statusListElements.push(<li key="iceConnectionState" className={iceConnectionState==="connected" || iceConnectionState==="completed"?okClass:errorClass} >iceConnectionState: {iceConnectionState || "none"}</li>);

    return (
      <div className="main">
        <div className="video">
          <video className="remote-video"  autoPlay ref={(video) => { this.remoteVideoElement = video; }} />
          <video className="local-video" autoPlay ref={(video) => { this.localVideoElement = video; }} />
        </div>

        <div className="status">
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

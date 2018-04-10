import React, { Component } from 'react';

import './Cockpit.css';


export class Cockpit extends Component {
  constructor(props) {
    super(props);
    this.localVideoElement = null;
    this.removeVideoElement = null;
    this.state = {
    };
  }

  render() {

    const hasConnectionToServer = this.props.connectedToServer;
    const hasConnectionToRobot = this.props.remoteVideoStream;
    const hasCamera = this.props.localVideoSteam;
    const okClass = "ok";
    const errorClass = "error";
    return (
      <div className="main">
        <div className="video">
          <video className="remote-video"  autoPlay ref={(video) => { this.localVideoElement = video; }} />
          <video className="local-video" autoPlay ref={(video) => { this.localVideoElement = video; }} />
        </div>



        <div className="status">
          <h3>Status:</h3>
            <ul>
              <li className={hasCamera?okClass:errorClass}             >Camera</li>
              <li className={hasConnectionToServer?okClass:errorClass} >Server</li>
              <li className={hasConnectionToRobot?okClass:errorClass}  >Robot</li>
            </ul>
        </div>
      </div>
      );
  }

  componentDidMount = () => {
    if (this.props.localVideoSteam) {
      console.log("Setting this.videoElement.srcObject to " + this.props.localVideoSteam);
      this.localVideoElement.srcObject = this.props.localVideoSteam;
    }
  }

  componentDidUpdate = (prevProps, prevState, snapshot) => {
    if (prevProps.localVideoSteam !== this.props.localVideoSteam) {
      console.log("Setting this.videoElement.srcObject to " + this.props.localVideoSteam);
      this.localVideoElement.srcObject = this.props.localVideoSteam;
    }
  }

}

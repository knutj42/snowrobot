import React, { Component } from 'react';

import './Cockpit.css';


export class Cockpit extends Component {
  constructor(props) {
    super(props);
    this.videoElement = null;
    this.state = {
    };
  }

  render() {

    const hasConnectionToServer = this.props.hasConnectionToServer;
    const hasConnectionToRobot = this.props.hasConnectionToRobot;
    const hasCamera = this.props.localVideoSteam;
    const okClass = "ok";
    const errorClass = "error";
    return (
      <div>
        <h2>Cockpit component</h2>

        <div>
          <video autoPlay ref={(video) => { this.videoElement = video; }} />
        </div>

        <div className="status">
          <h3>Status:</h3>
            <ul>
              <li className={hasCamera?okClass:errorClass}>Camera</li>
              <li className={hasConnectionToServer?okClass:errorClass}>Server</li>
              <li className={hasConnectionToRobot?okClass:errorClass}>Robot</li>
            </ul>
        </div>
      </div>
      );
  }

  componentDidMount = () => {
    if (this.props.localVideoSteam) {
      console.log("Setting this.videoElement.srcObject to " + this.props.localVideoSteam);
      this.videoElement.srcObject = this.props.localVideoSteam;
    }
  }

  componentDidUpdate = (prevProps, prevState, snapshot) => {
    if (prevProps.localVideoSteam !== this.props.localVideoSteam) {
      console.log("Setting this.videoElement.srcObject to " + this.props.localVideoSteam);
      this.videoElement.srcObject = this.props.localVideoSteam;
    }
  }

}

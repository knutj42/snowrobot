import React, { Component } from 'react';

import './Dashboard.css';


export class Dashboard extends Component {
  constructor(props) {
    super(props);
    this.state = {
      kp: 0,
      ki: 0,
      kd: 0,
    };
  }


  onUpdatePIDClicked = () => {
    if (this.props.dataChannelIsOpen) {
      console.log("Sending PID message.");

      this.props.dataChannel.send(JSON.stringify({
        "type": "pid",
        "kp": this.state.kp,
        "ki": this.state.ki,
        "kd": this.state.kd,
      }));
    }
  }

  render = () => {
    const robotVideoInputs = [];
    //const robotAudioInputs = [];
    //const robotAudioOutputs = [];

    const robotMediaDevices = this.props.robotMediaDevices;
    if (robotMediaDevices) {
      for (let i = 0; i !== robotMediaDevices.length; ++i) {
        const deviceInfo = robotMediaDevices[i];
        if (deviceInfo.kind === 'audioinput') {
          /*audioInputOptions.push(
            <option key={deviceInfo.deviceId} value={deviceInfo.deviceId}>
              {deviceInfo.label || 'microphone ' + (audioInputOptions.length)}
            </option>
          );*/
        } else if (deviceInfo.kind === 'audiooutput') {
          /*audioOutputOptions.push(
            <option key={deviceInfo.deviceId} value={deviceInfo.deviceId}>
              {deviceInfo.label || 'speaker ' + (audioOutputOptions.length)}
            </option>
          );*/

        } else if (deviceInfo.kind === 'videoinput') {
          /*videoInputOptions.push(
            <option key={deviceInfo.deviceId} value={deviceInfo.deviceId}>
              {deviceInfo.label || 'camera ' + (videoInputOptions.length)}
            </option>
          );*/
          const label = deviceInfo.label || 'camera ' + robotVideoInputs.length;
          robotVideoInputs.push(
          <button
            key={label}
            disabled={this.props.selectedRobotVideoSource === deviceInfo.deviceId}
            onClick={(event)=> {
                event.preventDefault();
                this.props.setMainState({selectedRobotVideoSource:deviceInfo.deviceId});
              }
            }
          >
            {label}
          </button>);

        } else {
          console.log('Some other kind of source/device: ', deviceInfo);
        }
      }
    }

    let robotHardwareErrorMsg = null;
    if (this.props.robotHardwareErrorMsg && this.props.robotHardwareErrorMsg !== 'null') {
      robotHardwareErrorMsg = (
        <div>
          Hardware error: {this.props.robotHardwareErrorMsg}
        </div>
      );
    }

    let pingMsg = null;
    if (this.props.lastPingRoundTripTime == null ) {
      pingMsg = "*no ping response*";
    } else {
      pingMsg = "ping roundtrip time: " + this.props.lastPingRoundTripTime + "ms";
    }


    return (
      <div className="dashboard">
        {robotVideoInputs}
        {robotHardwareErrorMsg}
        {pingMsg}
        <div>
          <input
            onChange={ (event) => {this.setState({kp: parseFloat(event.target.value)})}}
            type="number" step="0.001" min="0"
            name="kp" value={this.state.kp} />

          <input
            onChange={ (event) => {this.setState({ki: parseFloat(event.target.value)})}}
            type="number" step="0.001" min="0"
            name="ki" value={this.state.ki} />

          <input
            onChange={ (event) => {this.setState({kd: parseFloat(event.target.value)})}}
            type="number" step="0.001" min="0"
            name="kd" value={this.state.kd} />
          <button onClick={this.onUpdatePIDClicked}>Update PID</button>
        </div>
      </div>
    );
  }
}

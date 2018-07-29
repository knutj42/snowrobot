import React, { Component } from 'react';

import './Dashboard.css';


export class Dashboard extends Component {
  constructor(props) {
    super(props);
    this.state = {
    };
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

    return (
      <div className="dashboard">
        {robotVideoInputs}
        {robotHardwareErrorMsg}
      </div>
    );
  }
}

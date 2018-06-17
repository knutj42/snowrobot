import React, { Component } from 'react';

import './Configuration.css';

export class Configuration extends Component {
  onVideoSourceChanged = (event) => {
    localStorage.setItem("selectedVideoSource", event.target.value)
    this.props.setMainState({selectedVideoSource: event.target.value});
  }

  onAudioSourceChanged = (event) => {
    localStorage.setItem("selectedAudioSource", event.target.value)
    this.props.setMainState({selectedAudioSource: event.target.value});
  }

  onAudioOutputChanged = (event) => {
    localStorage.setItem("selectedAudioOutput", event.target.value)
    this.props.setMainState({selectedAudioOutput: event.target.value});
  }

  onRobotVideoSourceChanged = (event) => {
    this.props.setMainState({selectedRobotVideoSource: event.target.value});
  }

  onRobotAudioSourceChanged = (event) => {
    this.props.setMainState({selectedRobotAudioSource: event.target.value});
  }

  onRobotAudioOutputChanged = (event) => {
    this.props.setMainState({selectedRobotAudioOutput: event.target.value});
  }

  render() {

    const videoInputOptions = [
      (<option key="none" value="">
         *Select which video source to use*
       </option>
       ),
    ];

    const audioInputOptions = [
      (<option key="none" value="">
         *Select which microphone to use*
       </option>
       ),
    ];

    const audioOutputOptions = [
      (<option key="none" value="">
         *Select which audio output to use*
       </option>
       ),
    ];
    const mediaDevices = this.props.mediaDevices;
    if (mediaDevices) {
      for (let i = 0; i !== mediaDevices.length; ++i) {
        const deviceInfo = mediaDevices[i];
        if (deviceInfo.kind === 'audioinput') {
          audioInputOptions.push(
            <option key={deviceInfo.deviceId} value={deviceInfo.deviceId}>
              {deviceInfo.label || 'microphone ' + (audioInputOptions.length)}
            </option>
          );
        } else if (deviceInfo.kind === 'audiooutput') {
          audioOutputOptions.push(
            <option key={deviceInfo.deviceId} value={deviceInfo.deviceId}>
              {deviceInfo.label || 'speaker ' + (audioOutputOptions.length)}
            </option>
          );

        } else if (deviceInfo.kind === 'videoinput') {
          videoInputOptions.push(
            <option key={deviceInfo.deviceId} value={deviceInfo.deviceId}>
              {deviceInfo.label || 'camera ' + (videoInputOptions.length)}
            </option>
          );

        } else {
          console.log('Some other kind of source/device: ', deviceInfo);
        }
      }
    }


    const robotMediaDevices = this.props.robotMediaDevices;
    let robotSettings = null;
    if (robotMediaDevices) {
      const robotVideoInputOptions = [
        (<option key="none" value="">
           *Select which video source to use on the robot*
         </option>
         ),
      ];

      const robotAudioInputOptions = [
        (<option key="none" value="">
           *Select which microphone to use on the robot*
         </option>
         ),
      ];

      const robotAudioOutputOptions = [
        (<option key="none" value="">
           *Select which audio output to use on the robot*
         </option>
         ),
      ];

      for (let i = 0; i !== robotMediaDevices.length; ++i) {
        const deviceInfo = robotMediaDevices[i];
        if (deviceInfo.kind === 'audioinput') {
          robotAudioInputOptions.push(
            <option key={deviceInfo.deviceId} value={deviceInfo.deviceId}>
              {deviceInfo.label || 'microphone ' + (robotAudioInputOptions.length)}
            </option>
          );
        } else if (deviceInfo.kind === 'audiooutput') {
          robotAudioOutputOptions.push(
            <option key={deviceInfo.deviceId} value={deviceInfo.deviceId}>
              {deviceInfo.label || 'speaker ' + (robotAudioOutputOptions.length)}
            </option>
          );

        } else if (deviceInfo.kind === 'videoinput') {
          robotVideoInputOptions.push(
            <option key={deviceInfo.deviceId} value={deviceInfo.deviceId}>
              {deviceInfo.label || 'camera ' + (robotVideoInputOptions.length)}
            </option>
          );

        } else {
          console.log('Some other kind of source/device: ', deviceInfo);
        }
      }

      robotSettings = (
        <div>
          <h2>Robot settings</h2>
          <div>
            <label>
            Camera: &nbsp;
            <select value={this.props.selectedRobotVideoSource || ""} onChange={this.onRobotVideoSourceChanged}>
              {robotVideoInputOptions}
            </select>
            </label>
          </div>
          <div>
            <label>
            Microphone: &nbsp;
            <select value={this.props.selectedRobotAudioSource || ""} onChange={this.onRobotAudioSourceChanged}>
              {robotAudioInputOptions}
            </select>
            </label>
          </div>
          <div>
            <label>
            Audio output: &nbsp;
            <select value={this.props.selectedRobotAudioOutput|| ""} onChange={this.onRobotAudioOutputChanged}>
              {robotAudioOutputOptions}
            </select>
            </label>
          </div>
        </div>
      );
    }

    return (
      <div>
        <div>
          <h2>Local settings</h2>
          <div>
            <label>
            Camera: &nbsp;
            <select value={this.props.selectedVideoSource || ""} onChange={this.onVideoSourceChanged}>
              {videoInputOptions}
            </select>
            </label>
          </div>
          <div>
            <label>
            Microphone: &nbsp;
            <select value={this.props.selectedAudioSource || ""} onChange={this.onAudioSourceChanged}>
              {audioInputOptions}
            </select>
            </label>
          </div>
          <div>
            <label>
            Audio output: &nbsp;
            <select value={this.props.selectedAudioOutput|| ""} onChange={this.onAudioOutputChanged}>
              {audioOutputOptions}
            </select>
            </label>
          </div>
        </div>

        {robotSettings}
      </div>
      );
  }
}

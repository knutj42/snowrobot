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

  render() {

    const videoInputOptions = [
      (<option key="none" value="">
         *Select which video source to use*
       </option>
       ),
    ];

    const audioInputOptions = [
      (<option key="none">
         *Select which microphone to use*
       </option>
       ),
    ];

    const audioOutputOptions = [
      (<option key="none">
         *Select which audio output to use*
       </option>
       ),
    ];
    const mediaDevices = this.props.mediaDevices;
    if (mediaDevices) {
      for (var i = 0; i !== mediaDevices.length; ++i) {
        var deviceInfo = mediaDevices[i];
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

    return (
      <div>
        <div>
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
            <select value={this.props.selectedMicrophone || ""} onChange={this.onAudioSourceChanged}>
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
      </div>
      );
  }
}

import React, { Component } from 'react';
import {throttle} from 'lodash';

import './MovementControl.css';


export class MovementControl extends Component {
  constructor(props) {
    super(props);
    this.sendMovementMessageTimer = null;
    this.state = {
      dragging: false,
      dragStartPos: null,
      movementX: 0,
      movementY: 0,
      height: 0,
      width: 0,
    };


  }

  // we could get away with not having this (and just having the listeners on
  // our div), but then the experience would be possibly be janky. If there's
  // anything w/ a higher z-index that gets in the way, then you're toast,
  // etc.
  componentDidUpdate = (props, state) => {
    if (this.state.dragging && !state.dragging) {
      document.addEventListener('mousemove', this.onMouseMove)
      document.addEventListener('mouseup', this.onMouseUp)
    } else if (!this.state.dragging && state.dragging) {
      document.removeEventListener('mousemove', this.onMouseMove)
      document.removeEventListener('mouseup', this.onMouseUp)
    }
    this.updateSize();
  }

  // calculate relative position to the mouse and set dragging=true
  onMouseDown = (e) =>  {
    // only left mouse button
    console.log("onMouseDown() running. e.button:" + e.button);

    if (e.button !== 0) return

    this.setState({
      dragging: true,
      dragStartPos: {
        x: e.pageX,
        y: e.pageY
      }
    })

    // Send a movementmessage a couple a times a second even if no movement has been made,
    // so that the robot knows that the  controller is alive.
    this.sendMovementMessageTimer = setInterval(this.sendMovementMessage,
     500
    );

    e.stopPropagation()
    e.preventDefault()
  }

  onMouseUp = (e) =>  {
    console.log("onMouseUp() running.");
    this.setState({
      dragging: false,
      dragStartPos: null,
      movementX: 0,
      movementY:0,
      throttle:0,
      steering:0,
    }, this.sendMovementMessage);

    clearInterval(this.sendMovementMessageTimer);
    this.sendMovementMessageTimer = null;


    e.stopPropagation()
    e.preventDefault()
  }

  onMouseMove = (e) => {
    console.log("onMouseMove() running.");
    if (!this.state.dragging) return;
    let movementX = e.pageX - this.state.dragStartPos.x;
    let movementY = e.pageY - this.state.dragStartPos.y;
    const maxXMovement = (this.state.mainWidth / 2) - (this.state.draggerWidth / 2);
    const maxYMovement = (this.state.mainHeight / 2) - (this.state.draggerHeight / 2);
    movementX = Math.max(-maxXMovement, Math.min(movementX, maxXMovement));
    movementY = Math.max(-maxYMovement, Math.min(movementY, maxYMovement));

    const throttle = -movementY / maxYMovement;
    const steering = movementX / maxXMovement;
    //console.log("throttle:" + throttle + "  steering:" + steering);
    this.setState({throttle, steering, movementX, movementY}, this.sendMovementMessage);
    e.stopPropagation()
    e.preventDefault()
  }


  onTouchStart = (e) => {
    console.log("onTouchStart() running.");
    e.stopPropagation()
    e.preventDefault();
    let touch = e.touches.item(0);
    this.setState({
      dragging: true,
      dragStartPos: {
        x: touch.pageX,
        y: touch.pageY
      }
    })

    // Send a movementmessage a couple a times a second even if no movement has been made,
    // so that the robot knows that the  controller is alive.
    this.sendMovementMessageTimer = setInterval(this.sendMovementMessage,
     500
    );

  }

  onTouchMove = (e) => {
    console.log("onTouchMove () running.");
    e.stopPropagation()
    e.preventDefault();
    let touch = e.touches.item(0);
    if (!this.state.dragging) return;
    let movementX = touch.pageX - this.state.dragStartPos.x;
    let movementY = touch.pageY - this.state.dragStartPos.y;
    const maxXMovement = (this.state.mainWidth / 2) - (this.state.draggerWidth / 2);
    const maxYMovement = (this.state.mainHeight / 2) - (this.state.draggerHeight / 2);
    movementX = Math.max(-maxXMovement, Math.min(movementX, maxXMovement));
    movementY = Math.max(-maxYMovement, Math.min(movementY, maxYMovement));

    const throttle = -movementY / maxYMovement;
    const steering = movementX / maxXMovement;
    //console.log("throttle:" + throttle + "  steering:" + steering);
    this.setState({throttle, steering, movementX, movementY}, this.sendMovementMessage);
  }

  onTouchEnd = (e) => {
    console.log("onTouchEnd () running.");
    this.onTouchCancelOrEnd();
  }

  onTouchCancel = (e) => {
    console.log("onTouchEnd () running.");
    this.onTouchCancelOrEnd();
  }


  onTouchCancelOrEnd = (e) => {
    this.setState({
      dragging: false,
      dragStartPos: null,
      movementX: 0,
      movementY:0,
      throttle:0,
      steering:0,
    }, this.sendMovementMessage);

    clearInterval(this.sendMovementMessageTimer);
    this.sendMovementMessageTimer = null;
  }


  sendMovementMessage = throttle(() => {
    if (this.props.dataChannelIsOpen) {
      if (this.props.dataChannel.bufferedAmount > 1000) {
        console.log("Skipping movement message, since the datachannel seems busy. dataChannel.bufferedAmount:" + this.props.dataChannel.bufferedAmount);
      } else {
        console.log("Sending movement message. dataChannel.bufferedAmount:" + this.props.dataChannel.bufferedAmount);

        this.props.dataChannel.send(JSON.stringify({
          "type": "movement-control",
          "steering": this.state.steering,
          "throttle": this.state.throttle,
        }));

      }
    } else {
        console.log("Skipping movement message, since the datachannel isn't open");
    }
  }, 100
  , {
    leading: true,
    trailing: true,
  })

  componentDidMount = () =>  {
    this.updateSize();
  }

  updateSize = () => {
    if (this.mainElement) {
      const newMainHeight = this.mainElement.clientHeight;
      const newMainWidth = this.mainElement.clientWidth;
      if (newMainHeight !== this.state.mainHeight || newMainWidth !== this.state.mainWidth) {
        this.setState({
          mainHeight: newMainHeight,
          mainWidth: newMainWidth,
        });
      }
    }
    if (this.draggerElement) {
      const newDraggerHeight = this.draggerElement.clientHeight;
      const newDraggerWidth = this.draggerElement.clientWidth;
      if (newDraggerHeight !== this.state.draggerHeight || newDraggerWidth !== this.state.draggerWidth) {
        this.setState({
          draggerHeight: newDraggerHeight,
          draggerWidth: newDraggerWidth,
        });
      }
    }
  }

  render = () => {
    return (
      <div ref={(mainElement) => this.mainElement = mainElement} className="movement-control">
        <div className="control-area">
          <div className="dragger-parent">
            <div ref={(draggerElement) => this.draggerElement = draggerElement}  className="dragger"
              onMouseDown={this.onMouseDown}
              onTouchStart={this.onTouchStart}
              onTouchMove={this.onTouchMove}
              onTouchEnd={this.onTouchEnd}
              onTouchCancel={this.onTouchCancel}


              style={{
                position: 'relative',
                top: this.state.movementY,
                left: this.state.movementX,
              }}
            >
            </div>
          </div>
        </div>
      </div>
    );
  }
}

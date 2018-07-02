import React, { Component } from 'react';

import './MovementControl.css';


export class MovementControl extends Component {
  constructor(props) {
    super(props);
    this.state = {
      dragging: false,
      dragStartPos: null,
      movementX: 0,
      movementY: 0,
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
  }

  // calculate relative position to the mouse and set dragging=true
  onMouseDown = (e) =>  {
    // only left mouse button
    if (e.button !== 0) return

    this.setState({
      dragging: true,
      dragStartPos: {
        x: e.pageX,
        y: e.pageY
      }
    })
    e.stopPropagation()
    e.preventDefault()
  }

  onMouseUp = (e) =>  {
    this.setState({
      dragging: false,
      dragStartPos: null,
      movementX: 0,
      movementY:0,
    });
    e.stopPropagation()
    e.preventDefault()
  }

  onMouseMove = (e) => {
    if (!this.state.dragging) return
    const movementX = e.pageX - this.state.dragStartPos.x;
    const movementY = e.pageY - this.state.dragStartPos.y;
    this.setState({movementX, movementY});
    e.stopPropagation()
    e.preventDefault()
  }

  render = () => {
    return (
      <div className="movement-control">
      <div className="control-area">
        <div className="dragger-parent">
          <div className="dragger"
            onMouseDown={this.onMouseDown}
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

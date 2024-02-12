import unittest

import os
import time
import json

from utils import IntegrationTestBase

class SmokeTest(IntegrationTestBase):
    maxDiff = None

    def test_smoketest(self):
        reply = self.server_connection.send_message("ping")
        self.assertEqual(reply, "pong") 

        reply = self.client_connection.send_message("ping")
        self.assertEqual(reply, "pong")

        ###############################################################################
        # Wait for client to connect to the server.
        ###############################################################################
        starttime = time.monotonic()
        while True:
            is_server_connected_request = "is connected to server"
            reply = self.client_connection.send_message(is_server_connected_request)
            if reply == "true":
                break
            elapsed_time = time.monotonic() - starttime
            if elapsed_time > 60:
                raise AssertionError(
                    f"Timed out while waiting for the client to connect to the server. Last reply to the "
                    f"'{is_server_connected_request}' request was '{reply}'")
            time.sleep(1)

        ###############################################################################
        # Wait for the client to get a non-empty list of cameras from the server.
        ###############################################################################
        starttime = time.monotonic()
        cameras = None
        while True:
            get_cameras_request = "get cameras"
            reply = self.client_connection.send_message(get_cameras_request)
            try:
                cameras = json.loads(reply)
            except Exception as error:
                raise AssertionError(f"Failed to parse the reply to the 'get_cameras_request'! {reply=}  {error=}")
            if len(cameras) > 0:
                for camera in cameras:
                    resolutions = camera.get("resolutions")
                    if not resolutions:
                        raise AssertionError(f"Got a camera with no resolutions: {camera}")

                break
            elapsed_time = time.monotonic() - starttime
            if elapsed_time > 60:
                raise AssertionError(
                    f"Timed out while waiting for the client to get a list of cameras from server. Last reply to the "
                    f"'{get_cameras_request}' request was '{reply}'")
            time.sleep(1)

        #############################################################################################################
        # Tell the client to simulate that the user has selected the first camera and the camera's first resolution.
        #############################################################################################################
        selected_camera = cameras[0]
        for resolution in selected_camera["resolutions"]:
            if "video" in resolution:
                break
        select_camera_msg = json.dumps({
            "type": "select_camera",
            "camera": selected_camera["name"],
            "resolution": resolution,
        })
        reply = self.client_connection.send_message(select_camera_msg)
        self.assertEqual(reply, "ok")


if __name__ == "__main__":
    unittest.main()

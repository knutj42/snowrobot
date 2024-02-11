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


        starttime = time.monotonic()
        while True:
            get_cameras_request = "get cameras"
            reply = self.client_connection.send_message(get_cameras_request)
            try:
                cameras = json.loads(reply)
            except Exception as error:
                raise AssertionError(f"Failed to parse the reply to the 'get_cameras_request'! {reply=}  {error=}")
            if len(cameras) > 0:
                print(f"Got these cameras:\n{reply}")
                break
            elapsed_time = time.monotonic() - starttime
            if elapsed_time > 60:
                raise AssertionError(
                    f"Timed out while waiting for the client to get a list of cameras from server. Last reply to the "
                    f"'{get_cameras_request}' request was '{reply}'")
            time.sleep(1)



if __name__ == "__main__":
    unittest.main()

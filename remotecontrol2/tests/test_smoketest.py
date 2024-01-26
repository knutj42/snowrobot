import unittest

import os
import time

from utils import IntegrationTestBase

class SmokeTest(IntegrationTestBase):
    maxDiff = None

    def test_smoketest(self):
        reply = self.server_connection.send_message("ping")
        self.assertEqual(reply, "pong") 

        reply = self.client_connection.send_message("ping")
        self.assertEqual(reply, "pong") 



if __name__ == "__main__":
    unittest.main()

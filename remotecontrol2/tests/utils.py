import socket
import unittest

import os
import subprocess
import threading
import time



class DebugPortConnection:
    def __init__(self, host, port, timeout=60):

        # It can take a while for the process to start up, so we must be preparered to retry multiple time 
        starttime = time.monotonic()
        while True:
            try:
                self.socket = socket.create_connection((host, port))
                self.socket.settimeout(timeout)
                self.socket_file = self.socket.makefile(mode="r", encoding="utf-8")
                break
            except Exception as e:
                elapsedtime = time.monotonic() - starttime
                if elapsedtime > timeout:
                    raise
                time.sleep(0.1)

    def send_message(self, msg):
        self.socket.sendall((msg + "\n").encode("utf-8"))
        response = self.socket_file.readline()
        if response:
            response = response[:-1] # strip off newline
        return response


class IntegrationTestBase(unittest.TestCase):
    maxDiff = None
    def setUp(self):
        root_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
        self.source_dir = os.environ.get("CMAKE_SOURCE_DIR", os.path.join(root_path, "tests"))
        self.binary_dir = os.path.join(
            os.environ.get("CMAKE_BINARY_DIR", os.path.join(root_path, "build")),
            "remotecontrol2")

        server_executable_path = os.path.join(self.binary_dir, "server", "server")
        if not os.path.exists(server_executable_path):
            server_executable_path += ".exe"
        self.assertTrue(os.path.exists(server_executable_path), f"The server_path '{server_executable_path}' was not found!")
        self.server_process = subprocess.Popen(
            [server_executable_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            bufsize=1, universal_newlines=True)

        self.server_process_thread = threading.Thread(
            target=self.show_output,
              args=(self.server_process, f"{self._testMethodName}() server output: "))
        self.server_process_thread.start()

        self.server_connection = DebugPortConnection("localhost", 12345)


        client_executable_path = os.path.join(self.binary_dir, "client", "client")
        if not os.path.exists(client_executable_path):
            client_executable_path += ".exe"
        self.assertTrue(os.path.exists(client_executable_path), f"The client_path '{client_executable_path}' was not found!")
        self.client_process = subprocess.Popen(
            [client_executable_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            bufsize=1, universal_newlines=True)

        self.client_process_thread = threading.Thread(
            target=self.show_output,
              args=(self.client_process, f"{self._testMethodName}() client output: "))
        self.client_process_thread.start()

        self.client_connection = DebugPortConnection("localhost", 12346)

    def show_output(self, process, prefix):
        for line in process.stdout:
            line = line.rstrip()
            print(f"{prefix} - {line}")

    def tearDown(self):
        self.client_process.kill()
        self.server_process.kill()

        self.client_process.wait()
        self.server_process.wait()

        self.client_process_thread.join()
        self.server_process_thread.join()

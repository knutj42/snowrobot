from setuptools import setup

setup(
    name='Control Center',
    packages=['controlcenter'],
    include_package_data=True,
    install_requires=[
        'flask',
        'flask-socketio',
        'eventlet',
    ],
)
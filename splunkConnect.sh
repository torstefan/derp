#!/bin/bash
sudo ssh -L 80:localhost:8000 leafwiz@einbox.net -N&
google-chrome http://localhost/

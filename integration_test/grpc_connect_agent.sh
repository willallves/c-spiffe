#!/bin/bash
TOKEN=$(awk '{ print $2 }' /tmp/token.txt)
spire-agent run -joinToken $TOKEN -config /opt/spire/conf/agent/agent.conf &

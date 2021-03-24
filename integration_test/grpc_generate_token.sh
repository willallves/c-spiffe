#!/bin/bash
(spire-server token generate -spiffeID spiffe://example.org/host) > token.txt &

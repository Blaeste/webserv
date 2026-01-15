#!/bin/bash

echo "=== Testing parallel CGI execution ==="
echo "Launching 5 CGI scripts (each takes 3 seconds)..."
echo "If they run in parallel, total time should be ~3 seconds"
echo "If they run sequentially, total time would be ~15 seconds"
echo ""

start_time=$(date +%s)

# Launch 5 CGI requests in parallel
curl -s http://localhost:8080/cgi-bin/py/slow.py?id=1 > /tmp/cgi1.txt &
curl -s http://localhost:8080/cgi-bin/py/slow.py?id=2 > /tmp/cgi2.txt &
curl -s http://localhost:8080/cgi-bin/py/slow.py?id=3 > /tmp/cgi3.txt &
curl -s http://localhost:8080/cgi-bin/py/slow.py?id=4 > /tmp/cgi4.txt &
curl -s http://localhost:8080/cgi-bin/py/slow.py?id=5 > /tmp/cgi5.txt &

# Wait for all background jobs to complete
wait

end_time=$(date +%s)
elapsed=$((end_time - start_time))

echo "✓ All CGI completed in ${elapsed} seconds"
echo ""

# Check results
echo "=== Results ==="
for i in 1 2 3 4 5; do
    if grep -q "CGI #$i completed" /tmp/cgi${i}.txt 2>/dev/null; then
        echo "✓ CGI $i: SUCCESS"
    else
        echo "✗ CGI $i: FAILED"
    fi
done

echo ""
if [ $elapsed -le 5 ]; then
    echo "🎉 SUCCESS: CGI scripts ran in PARALLEL ($elapsed seconds)"
else
    echo "⚠️  WARNING: CGI scripts may have run sequentially ($elapsed seconds)"
fi

# Cleanup
rm -f /tmp/cgi*.txt

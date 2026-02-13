#!/usr/bin/env python3
import threading, sys, os

#Import daemon and dashboard
import daemon
import dashboard

def run_daemon():
	"""Launch daemon in a thread"""
	daemon.main()

def run_dashboard():
	"""Launch dashboard in a thread"""
	dashboard.app.run(host='0.0.0.0', port=5000, debug=False, use_reloader=False)

if __name__ == '__main__':
	print("🚀 Lancement Monitor (Daemon + Dashboard)")

	# Launch daemon
	daemon_thread = threading.Thread(target=run_daemon, daemon=True)
	daemon_thread.start()

	print("✅ Daemon démarré")
	print("📊 Dashboard démarrant sur http://localhost:5000")

	try:
		run_dashboard()
	except KeyboardInterrupt:
		print("\n🛑 Monitor arrêté")

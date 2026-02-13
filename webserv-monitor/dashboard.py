#!/usr/bin/env python3
from flask import Flask, jsonify, send_file, request
import sqlite3
import subprocess
import os
import threading

app = Flask(__name__)
DB_PATH = 'webserv_stats.db'
WEBSERV_ROOT = os.path.abspath('..')
test_running = {'status': None, 'output': ''}

def get_db():
	"""Connexion to Database"""
	return sqlite3.connect(DB_PATH, timeout=10.0)

@app.route('/')
def index():
	return send_file('static/dashboard.html')

@app.route('/api/stats')
def stats():
	conn = get_db()
	cursor = conn.cursor()

	# Total requests
	cursor.execute("SELECT COUNT(*) FROM requests")
	total = cursor.fetchone()[0]

	# Avg response time
	cursor.execute("SELECT AVG(response_time_us) FROM requests")
	avg_time = cursor.fetchone()[0] or 0

	# Error count (status >= 400)
	cursor.execute("SELECT COUNT(*) FROM requests WHERE status >= 400")
	errors = cursor.fetchone()[0]

	# Success rate
	success_rate = ((total - errors) / total * 100) if total > 0 else 0

	# Request per status
	cursor.execute("SELECT status, COUNT(*) FROM requests GROUP BY status")
	status_counts = {}
	for row in cursor.fetchall():
		status_counts[str(row[0])] = row[1]

	conn.close()

	return jsonify({
		'total_requests': total,
		'avg_response_time': round(avg_time, 2),
		'error_count': errors,
		'success_rate': round(success_rate, 2),
		'requests_per_status': status_counts
	})

@app.route('/api/requests')
def requests():
	conn = get_db()
	cursor = conn.cursor()

	cursor.execute("""
		SELECT timestamp, method, uri, status, size_bytes, response_time_us, client_ip
		FROM requests
		ORDER BY id DESC
		LIMIT 50
	""")

	rows = cursor.fetchall()
	conn.close()

	requests_list = []
	for row in rows:
		requests_list.append({
			'timestamp': row[0],
			'method': row[1],
			'uri': row[2],
			'status': row[3],
			'size_bytes': row[4],
			'response_time_us': row[5],
			'client_ip': row[6]
		})

	return jsonify({'requests': requests_list})

@app.route('/api/run_test', methods=['POST'])
def run_test():
	"""Lance le script de test Python seulement (webserv doit déjà tourner)"""
	global test_running

	if test_running['status'] == 'running':
		return jsonify({'error': 'Un test est déjà en cours'}), 400

	def run_command():
		global test_running
		test_running['status'] = 'running'
		test_running['output'] = ''
		try:
			# Vérifier que requests est installé
			subprocess.run(['python3', '-c', 'import requests'], check=True, capture_output=True)

			# Lancer le script de test
			result = subprocess.run(
				['python3', 'webServTester.py'],
				cwd=WEBSERV_ROOT,
				capture_output=True,
				text=True,
				timeout=300
			)
			test_running['output'] = result.stdout + result.stderr
			test_running['status'] = 'success' if result.returncode == 0 else 'error'
		except subprocess.CalledProcessError:
			test_running['output'] = "Erreur: package 'requests' manquant. Installer avec: pip install requests"
			test_running['status'] = 'error'
			test_running['status'] = 'error'

	threading.Thread(target=run_command, daemon=True).start()
	return jsonify({'message': 'Test lancé'})

@app.route('/api/run_eval', methods=['POST'])
def run_eval():
	"""Lance le tester d'eval seulement (webserv doit déjà tourner sur port 8080)"""
	global test_running

	if test_running['status'] == 'running':
		return jsonify({'error': 'Un test est déjà en cours'}), 400

	def run_command():
		global test_running
		test_running['status'] = 'running'
		test_running['output'] = ''
		try:
			# Télécharger le tester si nécessaire
			if not os.path.exists(os.path.join(WEBSERV_ROOT, 'tester')):
				subprocess.run(
					['wget', '-q', 'https://cdn.intra.42.fr/document/document/44506/tester'],
					cwd=WEBSERV_ROOT,
					timeout=30
				)
				subprocess.run(['chmod', '+x', 'tester'], cwd=WEBSERV_ROOT)

			# Lancer le tester
			result = subprocess.run(
				'yes "" | ./tester http://localhost:8080',
				cwd=WEBSERV_ROOT,
				shell=True,
				timeout=600
			)
			test_running['output'] = result.stdout + result.stderr
			test_running['status'] = 'success' if result.returncode == 0 else 'error'
		except Exception as e:
			test_running['output'] = str(e)
			test_running['status'] = 'error'

	threading.Thread(target=run_command, daemon=True).start()
	return jsonify({'message': 'Eval lancé'})

@app.route('/api/test_status')
def test_status():
	"""Retourne le statut du test en cours"""
	return jsonify(test_running)

@app.route('/api/clear_db', methods=['POST'])
def clear_db():
	"""Vide la base de données"""
	conn = get_db()
	cursor = conn.cursor()
	cursor.execute("DELETE FROM requests")
	conn.commit()
	conn.close()
	return jsonify({'message': 'Base de données vidée'})

if __name__ == '__main__':
	app.run(host="0.0.0.0", port=5000, debug=True)

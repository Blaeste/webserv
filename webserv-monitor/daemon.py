#!/usr/bin/env python3
import sys, re, sqlite3
from datetime import datetime

# Strip ANSI color codes before matching
ANSI_ESCAPE = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
LOG_PATTERN = r'(\w+):(\d+)\s+\|\s+\[([^\]]+)\]\s+\|\s+([^\s]+)\s+\|\s+(\w+)\s+([^\|]+)\|\s+(\d+\w+)\s+\|\s+→\s+(\d+)\s+\|\s+([^\s]+)'

def init_db():
	conn = sqlite3.connect('webserv_stats.db', timeout=10.0)
	cursor = conn.cursor()

	# Activer le mode WAL pour meilleures performances concurrentes
	cursor.execute('PRAGMA journal_mode=WAL')

	# Create table request
	cursor.execute('''CREATE TABLE IF NOT EXISTS requests (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		timestamp TEXT NOT NULL,
		server TEXT NOT NULL,
		port INTEGER NOT NULL,
		client_ip TEXT NOT NULL,
		method TEXT NOT NULL,
		uri TEXT NOT NULL,
		status INTEGER NOT NULL,
		size_bytes INTEGER NOT NULL,
		response_time_us REAL NOT NULL
	)''')

	# Create index
	cursor.execute('CREATE INDEX IF NOT EXISTS idx_timestamp ON requests(timestamp)')
	cursor.execute('CREATE INDEX IF NOT EXISTS idx_status ON requests(status)')

	conn.commit()
	return conn

def parse_log_line(line):
	# Line is already cleaned from ANSI codes in main()
	match = re.search(LOG_PATTERN, line)
	if not match:
		return None

	return {
		'server': match.group(1),
		'port': int(match.group(2)),
		'timestamp': match.group(3),
		'client_ip': match.group(4),
		'method': match.group(5),
		'uri': match.group(6).strip(),
		'size_str': match.group(7),
		'status': int(match.group(8)),
		'time_str': match.group(9)
	}

def parse_size(size_str):
	"""521B-> 521, 10K -> 10240"""
	if size_str.endswith('B'):
		return int(size_str[:-1])
	elif size_str.endswith('K'):
		return int(size_str[:-1]) * 1024
	elif size_str.endswith('M'):
		return int(size_str[:-1]) * 1024 * 1024
	return 0

def parse_time(time_str):
	"""85µs → 85.0, 1.2ms → 1200.0, 72-129µs → 129.0 (prend le max), 500.2ms → 500200.0"""
	time_str = time_str.strip()

	# Si c'est un intervalle (ex: "72-129µs"), prendre la valeur max
	if '-' in time_str:
		# Séparer la partie numérique de l'unité
		for unit in ['µs', 'ms', 's']:
			if unit in time_str:
				parts = time_str.replace(unit, '').split('-')
				time_str = parts[-1] + unit  # Prendre le max + unité
				break

	# Extraire l'unité et la valeur
	try:
		if 'µs' in time_str:
			return float(time_str.replace('µs', ''))
		elif 'ms' in time_str:
			return float(time_str.replace('ms', '')) * 1000
		elif time_str.endswith('s'):
			return float(time_str[:-1]) * 1000000
	except ValueError as e:
		print(f"Warning: impossible de parser '{time_str}': {e}", file=sys.stderr)
	return 0.0

def insert_request(conn, data):
	cursor = conn.cursor()

	# Convert size and time
	size = parse_size(data['size_str'])
	time = parse_time(data['time_str'])

	# Insert in DB (sans commit immédiat)
	cursor.execute('''
		INSERT INTO requests (timestamp, server, port, client_ip, method, uri, status, size_bytes, response_time_us)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
	''', (
		data['timestamp'],
		data['server'],
		data['port'],
		data['client_ip'],
		data['method'],
		data['uri'],
		data['status'],
		size,
		time
	))
	# Ne pas commit à chaque insert, on le fera par batch

# TODO: Main Loop - lire stdin
def main():
	print("🚀 Démarrage daemon", file=sys.stderr)
	sys.stderr.flush()
	conn = init_db()

	batch_count = 0
	BATCH_SIZE = 10  # Commit toutes les 10 requêtes

	try:
		buffer = ""

		while True:
			char = sys.stdin.read(1)
			if not char:
				break

			buffer += char

			# Détecter fin de ligne: séquence \033[K
			if buffer.endswith('\x1b[K'):
				# Nettoyer: enlever \r au début et \033[K à la fin
				line = buffer
				if line.startswith('\r'):
					line = line[1:]
				if line.endswith('\x1b[K'):
					line = line[:-3]

				# Supprimer les codes ANSI et strip
				clean_line = ANSI_ESCAPE.sub('', line).strip()

				# Ignorer lignes vides et séparateurs
				if clean_line and not clean_line.startswith('---'):
					# Parser et stocker
					data = parse_log_line(clean_line)
					if data:
						insert_request(conn, data)
						batch_count += 1

						# Commit par batch pour réduire le verrouillage
						if batch_count >= BATCH_SIZE:
							conn.commit()
							batch_count = 0

						print(f"✓ {data['method']} {data['uri']} → {data['status']}", file=sys.stderr)
						sys.stderr.flush()

				buffer = ""

	except KeyboardInterrupt:
		print("\n🛑 Arrêt", file=sys.stderr)
	finally:
		conn.commit()  # Commit final
		conn.close()

if __name__ == '__main__':
	main()

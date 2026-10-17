import sqlite3
import threading
import os

class Database:
    def __init__(self, db_path):
        self.db_path = db_path
        self.lock = threading.Lock()
        self._init_db()

    def _get_conn(self):
        #check_same_thread=False (FUSE может обращаться из разных потоков)
        return sqlite3.connect(self.db_path, check_same_thread=False)

    def _init_db(self):
        with self.lock:
            conn = self._get_conn()
            cursor = conn.cursor()
            #таблица данных
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS files (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    path TEXT UNIQUE NOT NULL,
                    mode INTEGER NOT NULL,
                    uid INTEGER NOT NULL,
                    gid INTEGER NOT NULL,
                    size INTEGER DEFAULT 0,
                    atime REAL NOT NULL,
                    mtime REAL NOT NULL,
                    ctime REAL NOT NULL,
                    nlink INTEGER DEFAULT 1
                )
            ''')
            #чанки
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS file_data (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    file_id INTEGER NOT NULL,
                    chunk_index INTEGER NOT NULL,
                    data BLOB,
                    FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE,
                    UNIQUE(file_id, chunk_index)
                )
            ''')
            cursor.execute('CREATE INDEX IF NOT EXISTS idx_files_path ON files(path)')
            cursor.execute('CREATE INDEX IF NOT EXISTS idx_file_data_file_id ON file_data(file_id)')
            conn.commit()
            conn.close()

    def get_file(self, path):
        with self.lock:
            conn = self._get_conn()
            cursor = conn.cursor()
            cursor.execute('SELECT * FROM files WHERE path = ?', (path,))
            row = cursor.fetchone()
            conn.close()
            if row:
                keys = ['id', 'path', 'mode', 'uid', 'gid', 'size', 'atime', 'mtime', 'ctime', 'nlink']
                return dict(zip(keys, row))
            return None

    def create_file(self, path, mode, uid, gid, is_dir=False):
        import time
        now = time.time()
        with self.lock:
            conn = self._get_conn()
            cursor = conn.cursor()
            try:
                cursor.execute('''
                    INSERT INTO files (path, mode, uid, gid, size, atime, mtime, ctime, nlink)
                    VALUES (?, ?, ?, ?, 0, ?, ?, ?, 1)
                ''', (path, mode, uid, gid, now, now, now))
                conn.commit()
            except sqlite3.IntegrityError:
                pass
            finally:
                conn.close()

    def delete_file(self, path):
        with self.lock:
            conn = self._get_conn()
            cursor = conn.cursor()
            cursor.execute('DELETE FROM files WHERE path = ?', (path,))
            conn.commit()
            conn.close()

    def list_dir(self, path):
        prefix = path if path.endswith('/') else path + '/'
        with self.lock:
            conn = self._get_conn()
            cursor = conn.cursor()
            #пути которые начинаются с prefix но без '/' после prefix
            cursor.execute('SELECT path FROM files WHERE path LIKE ? AND path != ?', (prefix + '%', path))
            rows = cursor.fetchall()
            conn.close()
            
            result = []
            for (p,) in rows:
                rel = p[len(prefix):]
                if '/' not in rel and rel != '':
                    result.append(rel)
            return result

    def write_data(self, path, data, offset):
        import time
        with self.lock:
            conn = self._get_conn()
            cursor = conn.cursor()
            cursor.execute('SELECT id, size FROM files WHERE path = ?', (path,))
            row = cursor.fetchone()
            if not row:
                conn.close()
                return -1
            file_id, current_size = row

            #читаем, модифицируем, пишем обратно
            cursor.execute('SELECT chunk_index, data FROM file_data WHERE file_id = ? ORDER BY chunk_index', (file_id,))
            chunks = cursor.fetchall()
            
            #текущие данные
            full_data = bytearray()
            for idx, chunk in chunks:
                full_data.extend(chunk)
            
            #расширяем (если надо)
            if len(full_data) < offset + len(data):
                full_data.extend(b'\x00' * (offset + len(data) - len(full_data)))
            
            #записываем
            full_data[offset:offset+len(data)] = data
            
            #перезаписываем чанк
            cursor.execute('DELETE FROM file_data WHERE file_id = ?', (file_id,))
            cursor.execute('INSERT INTO file_data (file_id, chunk_index, data) VALUES (?, ?, ?)', (file_id, 0, bytes(full_data)))
            
            #обновляем размер и mtime
            now = time.time()
            cursor.execute('UPDATE files SET size = ?, mtime = ? WHERE id = ?', (len(full_data), now, file_id))
            conn.commit()
            conn.close()
            return len(data)

    def read_data(self, path, size, offset):
        with self.lock:
            conn = self._get_conn()
            cursor = conn.cursor()
            cursor.execute('SELECT id FROM files WHERE path = ?', (path,))
            row = cursor.fetchone()
            if not row:
                conn.close()
                return b''
            file_id = row[0]
            
            cursor.execute('SELECT data FROM file_data WHERE file_id = ? ORDER BY chunk_index', (file_id,))
            chunks = cursor.fetchall()
            conn.close()
            
            full_data = bytearray()
            for (chunk,) in chunks:
                full_data.extend(chunk)
            
            return bytes(full_data[offset:offset+size])
import os
import shutil
import time
import hashlib
import json
from dataclasses import dataclass, asdict
from enum import Enum
from typing import Optional, Dict
from pathlib import Path

class StorageTier(Enum):
    HOT = 'hot'
    WARM = 'warm'
    COLD = 'cold'

@dataclass
class FileMetadata:
    filename: str
    tier: StorageTier
    size: int
    created_at: float
    last_accessed: float
    access_count: int
    checksum: str

class HybridStorageManager:
    def __init__(self, base_path: str = "./storage_data"):
        self.paths = {
            StorageTier.HOT: Path(base_path) / "hot",
            StorageTier.WARM: Path(base_path) / "warm",
            StorageTier.COLD: Path(base_path) / "cold"
        }
        self.metadata_file = Path(base_path) / "metadata.json"
        self.metadata: Dict[str, FileMetadata] = {}
        self._ensure_dirs()
        self._load_metadata()

    def _ensure_dirs(self):
        for path in self.paths.values():
            path.mkdir(parents=True, exist_ok=True)
        self.metadata_file.parent.mkdir(parents=True, exist_ok=True)

    def _load_metadata(self):
        if self.metadata_file.exists():
            with open(self.metadata_file, 'r') as f:
                data = json.load(f)
                for filename, meta_dict in data.items():
                    meta_dict['tier'] = StorageTier(meta_dict['tier'])
                    self.metadata[filename] = FileMetadata(**meta_dict)

    def _save_metadata(self):
        with open(self.metadata_file, 'w') as f:
            data = {k: {**asdict(v), 'tier': v.tier.value} for k, v in self.metadata.items()}
            json.dump(data, f, indent=2)

    def put(self, filename: str, data: bytes) -> FileMetadata:
        #сохранить файл (HOT tier)
        path = self.paths[StorageTier.HOT] / filename
        path.write_bytes(data)
        
        meta = FileMetadata(
            filename=filename,
            tier=StorageTier.HOT,
            size=len(data),
            created_at=time.time(),
            last_accessed=time.time(),
            access_count=1,
            checksum=hashlib.md5(data).hexdigest()
        )
        self.metadata[filename] = meta
        self._save_metadata()
        return meta

    def get(self, filename: str) -> Optional[bytes]:
        #получить файл (с авто promote на HOT)
        if filename not in self.metadata:
            return None
        
        meta = self.metadata[filename]
        
        #авто promote если файл не на HOT
        if meta.tier != StorageTier.HOT:
            print(f"[PROMOTE] Возвращаем '{filename}' с {meta.tier.value} на HOT")
            self._move_tier(filename, StorageTier.HOT)
            meta.tier = StorageTier.HOT
            
        path = self.paths[StorageTier.HOT] / filename
        meta.last_accessed = time.time()
        meta.access_count += 1
        self._save_metadata()
        return path.read_bytes()

    def migrate(self, hot_days=7, warm_days=30):
        #миграция данных(для демо секунды вместо дней)
        now = time.time()
        migrated = 0
        
        for filename, meta in list(self.metadata.items()):
            age = now - meta.last_accessed
            
            #в демо: 1 день = 5 секунд
            HOT_LIMIT = 5
            WARM_LIMIT = 15

            if meta.tier == StorageTier.HOT and age > HOT_LIMIT:
                self._move_tier(filename, StorageTier.WARM)
                meta.tier = StorageTier.WARM
                migrated += 1
                print(f"[MIGRATE] '{filename}' -> WARM (age: {int(age)}s)")
                
            elif meta.tier == StorageTier.WARM and age > WARM_LIMIT:
                self._move_tier(filename, StorageTier.COLD)
                meta.tier = StorageTier.COLD
                migrated += 1
                print(f"[MIGRATE] '{filename}' -> COLD (age: {int(age)}s)")
                
        if migrated > 0:
            self._save_metadata()
        else:
            print("нет файлов для миграции")

    def _move_tier(self, filename: str, target_tier: StorageTier):
        src = self.paths[self.metadata[filename].tier] / filename
        dst = self.paths[target_tier] / filename
        if src.exists():
            shutil.move(str(src), str(dst))

    def status(self):
        print("\n--- статус гибридного склада ---")
        for tier in StorageTier:
            files = [k for k, v in self.metadata.items() if v.tier == tier]
            print(f"{tier.value.upper()} tier ({self.paths[tier]}): {len(files)} файлов")
            for f in files:
                print(f"  - {f} (size: {self.metadata[f].size} bytes, accessed: {self.metadata[f].access_count} times)")
        print("-----------------------------------\n")

if __name__ == "__main__":
    mgr = HybridStorageManager()
    
    print("1) сохраняем файлы в HOT tier")
    mgr.put("video1.mp4", b"x" * 1024 * 1024 * 5) # 5 MB
    mgr.put("document.pdf", b"y" * 1024)          # 1 KB
    mgr.status()

    print("2) ждем 6 секунд (файлы стареют)")
    time.sleep(6)
    mgr.migrate()
    mgr.status()

    print("3) ждем 10 секунд (файлы стареют сильнее)")
    time.sleep(10)
    mgr.migrate()
    mgr.status()

    print("4) запрашиваем старый файл video1.mp4 (должен вернуться с COLD/WARM на HOT)")
    data = mgr.get("video1.mp4")
    print(f"   прочитано байт: {len(data) if data else 0}")
    mgr.status()
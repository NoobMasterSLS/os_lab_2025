import boto3
import time
import os
import json
import matplotlib.pyplot as plt
import numpy as np

#настройки для MinIO
S3_ENDPOINT = "http://localhost:9000"
ACCESS_KEY = "minioadmin"
SECRET_KEY = "minioadmin123"
BUCKET_NAME = "benchmark-bucket"
LOCAL_DIR = "/tmp/local_benchmark"

def setup_minio():
    s3 = boto3.client('s3', endpoint_url=S3_ENDPOINT, aws_access_key_id=ACCESS_KEY, aws_secret_access_key=SECRET_KEY)
    try:
        s3.create_bucket(Bucket=BUCKET_NAME)
    except Exception:
        pass
    return s3

def benchmark_io(iterations=20, file_size_kb=1024):
    data = os.urandom(file_size_kb * 1024)
    
    results = {
        "S3 (boto3)": {"write": [], "read": []},
        "Local FS": {"write": [], "read": []}
    }

    s3 = setup_minio()
    os.makedirs(LOCAL_DIR, exist_ok=True)
    local_file = os.path.join(LOCAL_DIR, "testfile.bin")

    print("запуск бенчмарка (запись/чтение)...")
    for i in range(iterations):
        #S3 API
        start = time.perf_counter()
        s3.put_object(Bucket=BUCKET_NAME, Key=f"file_{i}.bin", Body=data)
        results["S3 (boto3)"]["write"].append(time.perf_counter() - start)

        start = time.perf_counter()
        s3.get_object(Bucket=BUCKET_NAME, Key=f"file_{i}.bin")["Body"].read()
        results["S3 (boto3)"]["read"].append(time.perf_counter() - start)

        #локальные FS
        start = time.perf_counter()
        with open(local_file, 'wb') as f:
            f.write(data)
        results["Local FS"]["write"].append(time.perf_counter() - start)

        start = time.perf_counter()
        with open(local_file, 'rb') as f:
            f.read()
        results["Local FS"]["read"].append(time.perf_counter() - start)

    #расчет метрик
    summary = {}
    for name, ops in results.items():
        w_avg = np.mean(ops["write"]) * 1000
        r_avg = np.mean(ops["read"]) * 1000
        w_p95 = np.percentile(ops["write"], 95) * 1000
        r_p95 = np.percentile(ops["read"], 95) * 1000
        summary[name] = {
            "write_avg_ms": round(w_avg, 2),
            "read_avg_ms": round(r_avg, 2),
            "write_p95_ms": round(w_p95, 2),
            "read_p95_ms": round(r_p95, 2)
        }
    
    return summary

def plot_results(summary):
    labels = list(summary.keys())
    write_avg = [summary[l]["write_avg_ms"] for l in labels]
    read_avg = [summary[l]["read_avg_ms"] for l in labels]
    
    x = np.arange(len(labels))
    width = 0.35
    
    fig, ax = plt.subplots(figsize=(10, 6))
    rects1 = ax.bar(x - width/2, write_avg, width, label='Write (Avg)', color='#3498db')
    rects2 = ax.bar(x + width/2, read_avg, width, label='Read (Avg)', color='#e74c3c')
    
    ax.set_ylabel('Latency (ms)')
    ax.set_title('S3 API vs Local FS (1MB files)')
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.legend()
    
    ax.bar_label(rects1, padding=3)
    ax.bar_label(rects2, padding=3)
    
    plt.tight_layout()
    plt.savefig('benchmark_results.png', dpi=150)
    print("график сохранен в benchmark_results.png")

if __name__ == "__main__":
    print("ожидание запуска MinIO...")
    time.sleep(3) #время на старт MinIO
    results = benchmark_io()
    print("\nРезультаты (мс):")
    print(json.dumps(results, indent=2, ensure_ascii=False))
    plot_results(results)
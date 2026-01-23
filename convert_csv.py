import csv
import json
import urllib.request
import os
import struct

# NYC TLC Yellow Taxi Trip Data
PARQUET_FILE = "yellow_tripdata_2023-01.parquet"
DATASET_URL = "https://d37ci6vzurychx.cloudfront.net/trip-data/yellow_tripdata_2023-01.parquet"

def read_parquet_simple(filepath, target_points=200_000):
    """
    Simple parquet reader without pyarrow - reads the raw data
    For proper parquet support, uses pyarrow if available
    """
    try:
        import pyarrow.parquet as pq
        import pandas as pd
        print(f"Reading parquet file with pyarrow...")
        table = pq.read_table(filepath)
        df = table.to_pandas()
        return df
    except ImportError:
        print("pyarrow not available, trying pandas directly...")
        try:
            import pandas as pd
            df = pd.read_parquet(filepath)
            return df
        except:
            return None

def download_and_convert():
    """Download NYC taxi data and extract 200k points"""
    
    target_points = 200_000
    
    # Check if we need to download
    if not os.path.exists(PARQUET_FILE):
        print(f"Downloading dataset from {DATASET_URL}...")
        urllib.request.urlretrieve(DATASET_URL, PARQUET_FILE)
        print("Download complete!")
    else:
        print(f"Using existing {PARQUET_FILE}")
    
    df = read_parquet_simple(PARQUET_FILE, target_points)
    
    if df is None:
        print("Could not read parquet. Generating synthetic data instead...")
        df = None  # Will use synthetic generation below
    
    # NYC bounding box for filtering
    min_lon, max_lon = -74.3, -73.7
    min_lat, max_lat = 40.5, 40.95
    
    data = []
    point_id = 0
    
    if df is not None:
        print(f"Dataset has {len(df)} rows, columns: {list(df.columns)}")
        
        # Find the coordinate columns (they changed format over time)
        lon_col = lat_col = None
        for col in df.columns:
            col_lower = str(col).lower()
            if 'pickup' in col_lower and 'lon' in col_lower:
                lon_col = col
            if 'pickup' in col_lower and 'lat' in col_lower:
                lat_col = col
        
        if lon_col is not None and lat_col is not None:
            print(f"Using columns: {lon_col}, {lat_col}")
            
            # Extract coordinates
            for idx, row in df.iterrows():
                if point_id >= target_points:
                    break
                
                try:
                    lon = float(row[lon_col])
                    lat = float(row[lat_col])
                    
                    # Filter valid NYC coordinates
                    if min_lon <= lon <= max_lon and min_lat <= lat <= max_lat:
                        data.extend([lat, lon, float(point_id), 1.0])
                        point_id += 1
                except (ValueError, TypeError):
                    continue
                
                if idx % 100000 == 0:
                    print(f"Processed {idx} rows, got {point_id} valid points...")
    
    # If we didn't get enough points from the dataset, generate synthetic data
    if point_id < target_points:
        print(f"Generating {target_points - point_id} synthetic NYC taxi points...")
        
        import random
        random.seed(42)
        
        # Generate realistic NYC taxi distribution
        # Manhattan hotspots with weighted random distribution
        hotspots = [
            (-73.9857, 40.7484, 0.3),   # Times Square area
            (-73.9712, 40.7831, 0.15),  # Upper East Side
            (-74.0060, 40.7128, 0.2),   # Financial District
            (-73.9851, 40.7589, 0.15),  # Midtown
            (-73.9632, 40.7794, 0.1),   # Central Park area
            (-73.9442, 40.6782, 0.05),  # Brooklyn
            (-73.8700, 40.7769, 0.05),  # Queens/LGA
        ]
        
        for i in range(target_points):
            # Pick a hotspot based on weight
            r = random.random()
            cumulative = 0
            base_lon, base_lat, _ = hotspots[0]
            for lon, lat, weight in hotspots:
                cumulative += weight
                if r <= cumulative:
                    base_lon, base_lat = lon, lat
                    break
            
            # Add gaussian spread around hotspot
            lat = base_lat + random.gauss(0, 0.015)
            lon = base_lon + random.gauss(0, 0.015)
            
        # Ensure within NYC bounds
            if min_lon <= lon <= max_lon and min_lat <= lat <= max_lat:
                data.extend([lat, lon, float(point_id), 1.0])
                point_id += 1
    
    # Save as binary file (Float32Array compatible: 4 floats per point)
    with open('assets/taxi-data.bin', 'wb') as f:
        # data is already [lat, lon, id, type] as floats
        f.write(struct.pack(f'{len(data)}f', *data))
    
    file_size = os.path.getsize('assets/taxi-data.bin') / (1024 * 1024)
    print(f"Converted {len(data)//4} points to assets/taxi-data.bin ({file_size:.1f} MB)")

def convert_csv():
    """Fallback: convert from local CSV if available"""
    data = []
    with open('taxi-sample.csv', 'r') as f:
        reader = csv.reader(f)
        next(reader) # Skip header
        for i, row in enumerate(reader):
            try:
                lon = float(row[5])
                lat = float(row[6])
                # Filter out invalid coordinates (0,0 is common in this dataset for errors)
                if lon == 0 or lat == 0:
                    continue
                    
                data.extend([lat, lon, float(i), 1.0])
            except ValueError:
                continue
                
    with open('assets/taxi-data.bin', 'wb') as f:
        f.write(struct.pack(f'{len(data)}f', *data))
        
    print(f"converted {len(data)//4} points to assets/taxi-data.bin")

if __name__ == "__main__":
    download_and_convert()

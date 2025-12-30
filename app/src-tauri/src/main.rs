
#![cfg_attr(
  all(not(debug_assertions), target_os = "windows"),
  windows_subsystem = "windows"
)]

use serde::{Deserialize, Serialize};
use mdns_sd::{ServiceDaemon, ServiceEvent};
use std::time::Duration;
use reqwest;

#[derive(Debug, Serialize, Deserialize, Clone)]
struct DiscoveredCross {
  id: String,
  hostname: String,
  ip_address: String,
  port: u16,
  online: bool,
}

#[derive(Debug, Serialize, Deserialize)]
struct CrossStatus {
  firmware_version: String,
  cross_id: u16,
  geo_id: u16,
  device_name: String,
  hostname: String,
  wifi_connected: bool,
  wifi_ssid: String,
  ip_address: String,
  rssi: i32,
  free_heap: u32,
  uptime: u32,
}

#[derive(Debug, Serialize, Deserialize)]
struct WiFiNetwork {
  ssid: String,
  rssi: i32,
  encryption: bool,
}

#[derive(Debug, Serialize, Deserialize)]
struct WiFiScanResponse {
  networks: Vec<WiFiNetwork>,
}

#[tauri::command]
async fn discover_crosses(timeout_seconds: u64) -> Result<Vec<DiscoveredCross>, String> {
  let mdns = ServiceDaemon::new()
    .map_err(|e| format!("Failed to create mDNS daemon: {}", e))?;
  
  let mut discovered = Vec::new();
  let timeout = Duration::from_secs(timeout_seconds);
  let start = std::time::Instant::now();
  
  // Try multiple service types to catch the cross devices
  let service_types = vec![
    "_http._tcp.local.",
    "_services._dns-sd._udp.local.",
  ];
  
  for service_type in service_types {
    println!("Browsing for service type: {}", service_type);
    
    match mdns.browse(service_type) {
      Ok(receiver) => {
        while start.elapsed() < timeout {
          if let Ok(event) = receiver.recv_timeout(Duration::from_millis(500)) {
            match event {
              ServiceEvent::ServiceResolved(info) => {
                let hostname = info.get_hostname().to_string();
                let port = info.get_port();
                
                println!("Resolved service: {} (port: {})", hostname, port);
                
                // Check if hostname matches pattern *.cross or cross-*
                let hostname_lower = hostname.to_lowercase();
                if hostname_lower.ends_with(".cross") || hostname_lower.ends_with(".cross.") 
                   || hostname_lower.contains("cross") {
                  // Extract ID from hostname - handle {id}.cross or cross-{id}.local
                  let id = if hostname_lower.ends_with(".cross") || hostname_lower.ends_with(".cross.") {
                    // Pattern: {id}.cross
                    hostname_lower
                      .trim_end_matches('.')
                      .strip_suffix(".cross")
                      .unwrap_or("unknown")
                      .to_string()
                  } else if let Some(idx) = hostname_lower.find("cross-") {
                    // Pattern: cross-{id}.local
                    let after_cross = &hostname_lower[idx + 6..];
                    after_cross
                      .split('.')
                      .next()
                      .unwrap_or("unknown")
                      .to_string()
                  } else {
                    "unknown".to_string()
                  };
                  
                  let ip = info.get_addresses()
                    .iter()
                    .next()
                    .map(|addr| addr.to_string())
                    .unwrap_or_else(|| "unknown".to_string());
                  
                  println!("Matched Cross: id={}, hostname={}, ip={}, port={}", id, hostname, ip, port);
                  
                  let cross = DiscoveredCross {
                    id: id.clone(),
                    hostname: hostname.clone(),
                    ip_address: ip,
                    port,
                    online: true,
                  };
                  
                  // Check if we already have this one
                  if !discovered.iter().any(|c: &DiscoveredCross| c.hostname == hostname) {
                    discovered.push(cross);
                  }
                }
              }
              ServiceEvent::ServiceFound(type_domain, fullname) => {
                println!("Service found: {} - {}", type_domain, fullname);
              }
              ServiceEvent::ServiceRemoved(_, fullname) => {
                println!("Service removed: {}", fullname);
              }
              _ => {}
            }
          }
        }
      }
      Err(e) => {
        println!("Error browsing {}: {}", service_type, e);
      }
    }
  }
  
  println!("Discovery complete. Found {} crosses", discovered.len());
  
  // Sort by ID
  discovered.sort_by(|a, b| a.id.cmp(&b.id));
  
  Ok(discovered)
}

#[tauri::command]
fn resolve_cross_direct(cross_id: String) -> Result<DiscoveredCross, String> {
  // Try both patterns: {id}.cross and cross-{id}.local
  let hostnames = vec![
    format!("{}.cross", cross_id),
    format!("cross-{}.local", cross_id),
  ];
  
  use std::net::ToSocketAddrs;
  
  for hostname in hostnames {
    println!("Attempting direct resolution of: {}", hostname);
    
    match format!("{}:80", hostname).to_socket_addrs() {
      Ok(mut addrs) => {
        if let Some(addr) = addrs.next() {
          println!("Resolved {} to {}", hostname, addr.ip());
          return Ok(DiscoveredCross {
            id: cross_id.clone(),
            hostname: hostname.clone(),
            ip_address: addr.ip().to_string(),
            port: 80,
            online: true,
          });
        }
      }
      Err(e) => {
        println!("Failed to resolve {}: {}", hostname, e);
      }
    }
  }
  
  Err(format!("Could not resolve cross ID {}", cross_id))
}

#[tauri::command]
fn test_connection(hostname: String) -> Result<String, String> {
  println!("Testing connection to: {}", hostname);
  
  use std::net::ToSocketAddrs;
  
  // Try port 80
  let addr_str = format!("{}:80", hostname);
  println!("Attempting to resolve: {}", addr_str);
  
  match addr_str.to_socket_addrs() {
    Ok(mut addrs) => {
      if let Some(addr) = addrs.next() {
        println!("Successfully resolved {} to {}", hostname, addr.ip());
        Ok(format!("Connected! Found {} at {}", hostname, addr.ip()))
      } else {
        Err(format!("Resolved {} but no addresses returned", hostname))
      }
    }
    Err(e) => {
      println!("Failed to resolve {}: {}", hostname, e);
      Err(format!("Failed to resolve {}: {}", hostname, e))
    }
  }
}

#[tauri::command]
async fn get_cross_status(ip_address: String) -> Result<CrossStatus, String> {
  let url = format!("http://{}:80/status", ip_address);
  println!("Fetching status from: {}", url);
  
  match reqwest::get(&url).await {
    Ok(response) => {
      match response.json::<CrossStatus>().await {
        Ok(status) => Ok(status),
        Err(e) => Err(format!("Failed to parse status: {}", e)),
      }
    }
    Err(e) => Err(format!("Failed to connect: {}", e)),
  }
}

#[tauri::command]
async fn scan_wifi_networks(ip_address: String) -> Result<Vec<WiFiNetwork>, String> {
  let url = format!("http://{}:80/wifi/scan", ip_address);
  println!("Scanning WiFi networks from: {}", url);
  
  match reqwest::get(&url).await {
    Ok(response) => {
      let status = response.status();
      println!("Response status: {}", status);
      
      // Get the raw response text first for debugging
      match response.text().await {
        Ok(text) => {
          println!("Raw response: {}", text);
          
          // Try to parse the JSON
          match serde_json::from_str::<WiFiScanResponse>(&text) {
            Ok(scan_result) => Ok(scan_result.networks),
            Err(e) => Err(format!("Failed to parse networks: {}. Response was: {}", e, text)),
          }
        }
        Err(e) => Err(format!("Failed to read response: {}", e)),
      }
    }
    Err(e) => Err(format!("Failed to scan: {}", e)),
  }
}

#[tauri::command]
async fn configure_wifi(ip_address: String, ssid: String, password: String) -> Result<String, String> {
  let url = format!("http://{}:80/wifi", ip_address);
  println!("Configuring WiFi on: {}", url);
  
  let client = reqwest::Client::new();
  let mut body = serde_json::Map::new();
  body.insert("ssid".to_string(), serde_json::Value::String(ssid));
  body.insert("password".to_string(), serde_json::Value::String(password));
  
  match client.post(&url)
    .json(&body)
    .send()
    .await {
    Ok(response) => {
      if response.status().is_success() {
        Ok("WiFi configured successfully".to_string())
      } else {
        Err(format!("Failed with status: {}", response.status()))
      }
    }
    Err(e) => Err(format!("Failed to configure: {}", e)),
  }
}

#[tauri::command]
async fn send_prayer(ip_address: String, prayer_data: serde_json::Value) -> Result<String, String> {
  let url = format!("http://{}:80/prayer", ip_address);
  println!("Sending prayer to: {}", url);
  
  let client = reqwest::Client::new();
  
  match client.post(&url)
    .json(&prayer_data)
    .send()
    .await {
    Ok(response) => {
      if response.status().is_success() {
        Ok("Prayer sent successfully".to_string())
      } else {
        Err(format!("Failed with status: {}", response.status()))
      }
    }
    Err(e) => Err(format!("Failed to send: {}", e)),
  }
}

fn main() {
  tauri::Builder::default()
    .invoke_handler(tauri::generate_handler![
      discover_crosses,
      resolve_cross_direct,
      test_connection,
      get_cross_status,
      scan_wifi_networks,
      configure_wifi,
      send_prayer
    ])
    .run(tauri::generate_context!())
    .expect("error while running tauri application");
}

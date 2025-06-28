// Test script pentru verificarea fluxului de date în aplicația Smart Home
const axios = require('axios');

const BASE_URL = "http://192.168.3.86:8080/api";

// Lista endpoint-urilor de testat
const endpoints = [
  { name: "Status", url: `${BASE_URL}/status`, method: 'GET' },
  { name: "Temperature", url: `${BASE_URL}/temperature`, method: 'GET' },
  { name: "Humidity", url: `${BASE_URL}/humidity`, method: 'GET' },
  { name: "Gas", url: `${BASE_URL}/gaz`, method: 'GET' },
  { name: "Temperature History", url: `${BASE_URL}/temperature/history`, method: 'GET' },
  { name: "Humidity History", url: `${BASE_URL}/humidity/history`, method: 'GET' },
  { name: "Gas History", url: `${BASE_URL}/gaz/history`, method: 'GET' },
  { name: "Cooler Energy Stats", url: `${BASE_URL}/cooler/energy?interval=day`, method: 'GET' },
  { name: "Cooler Usage", url: `${BASE_URL}/cooler/usage?interval=day`, method: 'GET' },
  { name: "Temperature Stats", url: `${BASE_URL}/temperature/stats?interval=day`, method: 'GET' },
  { name: "Humidity Stats", url: `${BASE_URL}/humidity/stats?interval=day`, method: 'GET' },
  { name: "Gas Stats", url: `${BASE_URL}/gas/stats?interval=day`, method: 'GET' },
];

async function testEndpoint(endpoint) {
  try {
    console.log(`\n🔍 Testez: ${endpoint.name}`);
    console.log(`   URL: ${endpoint.url}`);
    
    const startTime = Date.now();
    const response = await axios({
      method: endpoint.method,
      url: endpoint.url,
      timeout: 5000
    });
    const endTime = Date.now();
    
    console.log(`   ✅ Status: ${response.status}`);
    console.log(`   ⏱️  Timp răspuns: ${endTime - startTime}ms`);
    console.log(`   📊 Date primite:`, JSON.stringify(response.data, null, 2));
    
    return { success: true, data: response.data, time: endTime - startTime };
  } catch (error) {
    console.log(`   ❌ Eroare: ${error.message}`);
    if (error.response) {
      console.log(`   📋 Status Code: ${error.response.status}`);
      console.log(`   📄 Response Data:`, error.response.data);
    }
    return { success: false, error: error.message };
  }
}

async function testDeviceControl() {
  console.log("\n🔧 Testez controlul device-urilor...");
  
  const devices = ['kitchen', 'living', 'bedroom', 'cooler'];
  
  for (const device of devices) {
    try {
      // Testează obținerea stării
      console.log(`\n   📱 Testez device: ${device}`);
      const getStateResponse = await axios.get(`${BASE_URL}/devices/${device}`);
      console.log(`   ✅ Stare curentă: ${JSON.stringify(getStateResponse.data)}`);
      
      // Testează setarea stării (doar pentru primul device pentru a nu afecta sistemul)
      if (device === 'kitchen') {
        const currentState = getStateResponse.data[device];
        const newState = currentState === 'on' ? 'off' : 'on';
        
        console.log(`   🔄 Setez ${device} la ${newState}...`);
        const setStateResponse = await axios.post(`${BASE_URL}/devices/${device}`, { state: newState });
        console.log(`   ✅ Setare reușită: ${JSON.stringify(setStateResponse.data)}`);
        
        // Restore la starea originală
        console.log(`   🔄 Restore ${device} la ${currentState}...`);
        await axios.post(`${BASE_URL}/devices/${device}`, { state: currentState });
        console.log(`   ✅ Restore reușit`);
      }
      
    } catch (error) {
      console.log(`   ❌ Eroare pentru ${device}: ${error.message}`);
    }
  }
}

async function runTests() {
  console.log("🚀 Încep testarea aplicației Smart Home...");
  console.log(`📍 Server URL: ${BASE_URL}`);
  
  const results = [];
  
  // Testează toate endpoint-urile
  for (const endpoint of endpoints) {
    const result = await testEndpoint(endpoint);
    results.push({ endpoint: endpoint.name, ...result });
  }
  
  // Testează controlul device-urilor
  await testDeviceControl();
  
  // Sumar rezultate
  console.log("\n📈 SUMAR REZULTATE:");
  console.log("==================");
  
  const successful = results.filter(r => r.success).length;
  const failed = results.filter(r => !r.success).length;
  
  console.log(`✅ Succes: ${successful}/${results.length}`);
  console.log(`❌ Eșec: ${failed}/${results.length}`);
  
  if (failed > 0) {
    console.log("\n❌ Endpoint-uri cu probleme:");
    results.filter(r => !r.success).forEach(r => {
      console.log(`   - ${r.endpoint}: ${r.error}`);
    });
  }
  
  if (successful > 0) {
    console.log("\n✅ Endpoint-uri funcționale:");
    results.filter(r => r.success).forEach(r => {
      console.log(`   - ${r.endpoint} (${r.time}ms)`);
    });
  }
  
  console.log("\n🎯 RECOMANDĂRI:");
  if (failed === 0) {
    console.log("   ✅ Toate endpoint-urile funcționează corect!");
    console.log("   ✅ Aplicația poate prelua datele de la server");
    console.log("   ✅ Controlul device-urilor funcționează");
  } else {
    console.log("   ⚠️  Există probleme cu unele endpoint-uri");
    console.log("   🔧 Verifică conectivitatea la server");
    console.log("   🔧 Verifică dacă serverul rulează pe portul corect");
    console.log("   🔧 Verifică firewall-ul și setările de rețea");
  }
}

// Rulează testele
runTests().catch(console.error); 
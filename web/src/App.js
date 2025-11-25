import React, { useState, useEffect } from 'react';
import CurrentStatus from './components/CurrentStatus';
import UtilizationChart from './components/UtilizationChart';
import ModeToggle from './components/ModeToggle';
import './App.css';

function App() {
  const [currentStatus, setCurrentStatus] = useState(null);
  const [utilizationData, setUtilizationData] = useState([]);
  const [lastUpdate, setLastUpdate] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [currentMode, setCurrentMode] = useState('unknown');

  // Fetch current status
  const fetchCurrentStatus = async () => {
    try {
      const response = await fetch('/api/status');
      if (response.ok) {
        const data = await response.json();
        setCurrentStatus(data);
        setLastUpdate(new Date().toLocaleString());
        setError(null);
      } else {
        throw new Error(`HTTP ${response.status}`);
      }
    } catch (err) {
      setError(`Failed to fetch current status: ${err.message}`);
    }
  };

  // Fetch utilization data
  const fetchUtilizationData = async () => {
    try {
      const response = await fetch('/api/utilization');
      if (response.ok) {
        const data = await response.json();
        setUtilizationData(data);
        setError(null);
      } else {
        throw new Error(`HTTP ${response.status}`);
      }
    } catch (err) {
      setError(`Failed to fetch utilization data: ${err.message}`);
    }
  };

  // Initial data load
  useEffect(() => {
    const loadData = async () => {
      setLoading(true);
      await Promise.all([
        fetchCurrentStatus(),
        fetchUtilizationData()
      ]);
      setLoading(false);
    };

    loadData();
  }, []);

  // Auto-refresh current status every 5 seconds
  useEffect(() => {
    const interval = setInterval(fetchCurrentStatus, 5000);
    return () => clearInterval(interval);
  }, []);

  // Auto-refresh utilization data every 30 seconds
  useEffect(() => {
    const interval = setInterval(fetchUtilizationData, 30000);
    return () => clearInterval(interval);
  }, []);

  // Handle mode changes
  const handleModeChange = (newMode) => {
    setCurrentMode(newMode);
    // Refresh data after mode change
    setTimeout(() => {
      fetchCurrentStatus();
      fetchUtilizationData();
    }, 2000);
  };

  if (loading) {
    return (
      <div className="app">
        <div className="loading">Loading BoilerStat Dashboard...</div>
      </div>
    );
  }

  return (
    <div className="app">
      <header className="app-header">
        <h1>BoilerStat Dashboard</h1>
        <div className="header-info">
          <span>Last Update: {lastUpdate}</span>
          {error && <span className="error">⚠️ {error}</span>}
        </div>
      </header>

      <main className="app-main">
        <div className="dashboard-grid">
          <section className="mode-section">
            <ModeToggle onModeChange={handleModeChange} />
          </section>

          <section className="status-section">
            <h2>Current Status</h2>
            <CurrentStatus data={currentStatus} />
          </section>

          <section className="chart-section">
            <h2>1-Hour Utilization Trends</h2>
            <UtilizationChart data={utilizationData} />
          </section>
        </div>
      </main>
    </div>
  );
}

export default App;
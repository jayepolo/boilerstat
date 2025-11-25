import React, { useState, useEffect } from 'react';

const ModeToggle = ({ onModeChange }) => {
  const [currentMode, setCurrentMode] = useState('unknown');
  const [isChanging, setIsChanging] = useState(false);
  const [lastChanged, setLastChanged] = useState(null);

  // Fetch current mode on component mount
  useEffect(() => {
    fetchCurrentMode();
    // Poll mode every 10 seconds to stay in sync
    const interval = setInterval(fetchCurrentMode, 10000);
    return () => clearInterval(interval);
  }, []);

  const fetchCurrentMode = async () => {
    try {
      const response = await fetch('/api/mode');
      const data = await response.json();
      if (data.mode) {
        setCurrentMode(data.mode);
      }
    } catch (error) {
      console.error('Failed to fetch current mode:', error);
    }
  };

  const handleModeToggle = async () => {
    const newMode = currentMode === 'demo' ? 'production' : 'demo';
    setIsChanging(true);

    try {
      const response = await fetch('/api/mode', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ mode: newMode }),
      });

      const result = await response.json();
      
      if (response.ok && result.success) {
        setCurrentMode(newMode);
        setLastChanged(new Date().toLocaleTimeString());
        if (onModeChange) {
          onModeChange(newMode);
        }
      } else {
        console.error('Failed to change mode:', result.error);
        alert('Failed to change mode: ' + (result.error || 'Unknown error'));
      }
    } catch (error) {
      console.error('Error changing mode:', error);
      alert('Error changing mode: ' + error.message);
    } finally {
      setIsChanging(false);
    }
  };

  const getModeDisplayInfo = () => {
    switch (currentMode) {
      case 'demo':
        return {
          text: 'Demo Mode',
          icon: '🎭',
          description: 'Generating realistic mock data',
          className: 'mode-demo'
        };
      case 'production':
        return {
          text: 'Production Mode', 
          icon: '🏭',
          description: 'Reading actual GPIO pins',
          className: 'mode-production'
        };
      default:
        return {
          text: 'Unknown Mode',
          icon: '❓',
          description: 'Mode detection in progress...',
          className: 'mode-unknown'
        };
    }
  };

  const modeInfo = getModeDisplayInfo();

  return (
    <div className="mode-toggle-container horizontal">
      <div className="mode-status-horizontal">
        <div className={`mode-indicator-horizontal ${modeInfo.className}`}>
          <span className="mode-icon-horizontal">{modeInfo.icon}</span>
          <div className="mode-details-horizontal">
            <div className="mode-text-horizontal">{modeInfo.text}</div>
            <div className="mode-description-horizontal">{modeInfo.description}</div>
            {lastChanged && (
              <div className="mode-timestamp-horizontal">Changed at {lastChanged}</div>
            )}
          </div>
        </div>
      </div>
      
      <div className="mode-controls-horizontal">
        <button 
          className={`mode-toggle-button-horizontal ${isChanging ? 'changing' : ''}`}
          onClick={handleModeToggle}
          disabled={isChanging || currentMode === 'unknown'}
        >
          {isChanging ? (
            <>
              <span className="spinner">⟳</span>
              Switching...
            </>
          ) : (
            <>
              Switch to {currentMode === 'demo' ? 'Production' : 'Demo'}
            </>
          )}
        </button>
        
      </div>
    </div>
  );
};

export default ModeToggle;
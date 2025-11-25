import React, { useState, useEffect, useRef } from 'react';

const MessageWindow = () => {
  const [messages, setMessages] = useState([]);
  const [isConnected, setIsConnected] = useState(false);
  const [isPaused, setIsPaused] = useState(false);
  const [showDetails, setShowDetails] = useState(true);
  const messagesEndRef = useRef(null);
  const eventSourceRef = useRef(null);

  useEffect(() => {
    // Load initial messages
    fetchRecentMessages();
    
    // Set up Server-Sent Events for real-time updates
    connectEventSource();
    
    return () => {
      if (eventSourceRef.current) {
        eventSourceRef.current.close();
      }
    };
  }, []);

  useEffect(() => {
    // Auto-scroll to bottom when new messages arrive (if not paused)
    if (!isPaused && messagesEndRef.current) {
      messagesEndRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [messages, isPaused]);

  const fetchRecentMessages = async () => {
    try {
      const response = await fetch('/api/messages');
      const data = await response.json();
      setMessages(data);
    } catch (error) {
      console.error('Failed to fetch recent messages:', error);
    }
  };

  const connectEventSource = () => {
    try {
      eventSourceRef.current = new EventSource('/api/stream');
      
      eventSourceRef.current.onopen = () => {
        setIsConnected(true);
        console.log('Connected to ESP32 message stream');
      };
      
      eventSourceRef.current.onmessage = (event) => {
        if (!isPaused) {
          try {
            const newMessage = JSON.parse(event.data);
            setMessages(prev => [...prev.slice(-49), newMessage]); // Keep last 50 messages
          } catch (error) {
            console.error('Failed to parse message:', error);
          }
        }
      };
      
      eventSourceRef.current.onerror = () => {
        setIsConnected(false);
        console.error('Lost connection to ESP32 message stream');
        // Attempt to reconnect after 5 seconds
        setTimeout(connectEventSource, 5000);
      };
    } catch (error) {
      console.error('Failed to connect to message stream:', error);
      setIsConnected(false);
    }
  };

  const clearMessages = () => {
    setMessages([]);
  };

  const pauseToggle = () => {
    setIsPaused(!isPaused);
  };

  const formatZoneData = (zones) => {
    return zones.map((zone, index) => (
      <span key={index} className={`zone-value ${zone ? 'zone-on' : 'zone-off'}`}>
        {zone}
      </span>
    ));
  };

  const getMessageAge = (timestamp) => {
    const now = new Date();
    const msgTime = new Date(timestamp);
    const diffSeconds = Math.floor((now - msgTime) / 1000);
    
    if (diffSeconds < 60) return `${diffSeconds}s ago`;
    if (diffSeconds < 3600) return `${Math.floor(diffSeconds / 60)}m ago`;
    return msgTime.toLocaleTimeString();
  };

  return (
    <div className="message-window">
      <div className="message-header">
        <div className="header-left">
          <h3>📡 ESP32 Live Stream</h3>
          <div className={`connection-status ${isConnected ? 'connected' : 'disconnected'}`}>
            {isConnected ? '● Connected' : '○ Disconnected'}
          </div>
        </div>
        
        <div className="header-controls">
          <button 
            className={`control-btn ${showDetails ? 'active' : ''}`}
            onClick={() => setShowDetails(!showDetails)}
            title="Toggle detailed view"
          >
            {showDetails ? '📋' : '📄'}
          </button>
          <button 
            className={`control-btn ${isPaused ? 'paused' : ''}`}
            onClick={pauseToggle}
            title={isPaused ? 'Resume updates' : 'Pause updates'}
          >
            {isPaused ? '▶️' : '⏸️'}
          </button>
          <button 
            className="control-btn"
            onClick={clearMessages}
            title="Clear messages"
          >
            🗑️
          </button>
        </div>
      </div>

      <div className="message-content">
        {messages.length === 0 ? (
          <div className="no-messages">
            Waiting for ESP32 messages...
          </div>
        ) : (
          <div className="message-list">
            {messages.map((msg, index) => (
              <div 
                key={index} 
                className={`message-item ${msg.mode || 'unknown'}-mode`}
              >
                <div className="message-main">
                  <span className="message-time">{msg.received_at}</span>
                  <span className="message-timestamp">[{msg.timestamp}]</span>
                  <span className={`burner-indicator ${msg.burner ? 'on' : 'off'}`}>
                    🔥 {msg.burner ? 'ON' : 'OFF'}
                  </span>
                  <span className="zones-container">
                    🏠 {formatZoneData(msg.zones)}
                  </span>
                  {msg.mode && (
                    <span className={`mode-badge ${msg.mode}`}>
                      {msg.mode === 'demo' ? '🎭' : '🏭'}
                    </span>
                  )}
                </div>
                
                {showDetails && (
                  <div className="message-details">
                    <div className="detail-row">
                      <span>Burner: </span>
                      <span className={msg.burner ? 'value-on' : 'value-off'}>
                        {msg.burner}
                      </span>
                    </div>
                    <div className="detail-row">
                      <span>Zones: </span>
                      {msg.zones.map((zone, i) => (
                        <span key={i} className={`zone-detail ${zone ? 'on' : 'off'}`}>
                          Z{i+1}:{zone}
                        </span>
                      ))}
                    </div>
                    <div className="detail-row">
                      <span>Active Zones: </span>
                      <span>{msg.zones.filter(z => z).length}/6</span>
                    </div>
                  </div>
                )}
              </div>
            ))}
            <div ref={messagesEndRef} />
          </div>
        )}
      </div>

      {isPaused && (
        <div className="pause-overlay">
          ⏸️ Updates Paused - Click resume to continue
        </div>
      )}
    </div>
  );
};

export default MessageWindow;
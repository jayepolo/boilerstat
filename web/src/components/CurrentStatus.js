import React from 'react';

const CurrentStatus = ({ data }) => {
  if (!data) {
    return (
      <div className="current-status">
        <div className="status-item">
          <span>No data available</span>
        </div>
      </div>
    );
  }

  const formatTimestamp = (timestamp) => {
    try {
      const date = new Date(timestamp);
      return date.toLocaleString();
    } catch (e) {
      return timestamp;
    }
  };

  const getStatusClass = (value) => {
    return value ? 'active' : 'inactive';
  };

  const getValueClass = (value) => {
    return value ? 'on' : 'off';
  };

  const getValueText = (value) => {
    return value ? 'ON' : 'OFF';
  };

  return (
    <div className="current-status">
      <div className={`status-item ${getStatusClass(data.burner)}`}>
        <span className="status-label">🔥 Burner</span>
        <span className={`status-value ${getValueClass(data.burner)}`}>
          {getValueText(data.burner)}
        </span>
      </div>

      <div className={`status-item ${getStatusClass(data.zone_1)}`}>
        <span className="status-label">🏠 Zone 1</span>
        <span className={`status-value ${getValueClass(data.zone_1)}`}>
          {getValueText(data.zone_1)}
        </span>
      </div>

      <div className={`status-item ${getStatusClass(data.zone_2)}`}>
        <span className="status-label">🏠 Zone 2</span>
        <span className={`status-value ${getValueClass(data.zone_2)}`}>
          {getValueText(data.zone_2)}
        </span>
      </div>

      <div className={`status-item ${getStatusClass(data.zone_3)}`}>
        <span className="status-label">🏠 Zone 3</span>
        <span className={`status-value ${getValueClass(data.zone_3)}`}>
          {getValueText(data.zone_3)}
        </span>
      </div>

      <div className={`status-item ${getStatusClass(data.zone_4)}`}>
        <span className="status-label">🏠 Zone 4</span>
        <span className={`status-value ${getValueClass(data.zone_4)}`}>
          {getValueText(data.zone_4)}
        </span>
      </div>

      <div className={`status-item ${getStatusClass(data.zone_5)}`}>
        <span className="status-label">🏠 Zone 5</span>
        <span className={`status-value ${getValueClass(data.zone_5)}`}>
          {getValueText(data.zone_5)}
        </span>
      </div>

      <div className={`status-item ${getStatusClass(data.zone_6)}`}>
        <span className="status-label">🏠 Zone 6</span>
        <span className={`status-value ${getValueClass(data.zone_6)}`}>
          {getValueText(data.zone_6)}
        </span>
      </div>

      <div className="timestamp">
        <strong>Last Reading:</strong> {formatTimestamp(data.timestamp)}
      </div>
    </div>
  );
};

export default CurrentStatus;
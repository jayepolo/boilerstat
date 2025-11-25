import React, { useState } from 'react';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
} from 'chart.js';
import { Line } from 'react-chartjs-2';

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
);

const UtilizationChart = ({ data }) => {
  // State to track which data series are visible
  const [visibleSeries, setVisibleSeries] = useState({
    burner: true,
    zone_1: true,
    zone_2: true,
    zone_3: true,
    zone_4: true,
    zone_5: true,
    zone_6: true,
  });

  if (!data || data.length === 0) {
    return (
      <div className="chart-container">
        <div style={{ display: 'flex', justifyContent: 'center', alignItems: 'center', height: '100%' }}>
          <span>No utilization data available</span>
        </div>
      </div>
    );
  }

  // Function to toggle series visibility
  const toggleSeries = (seriesKey) => {
    setVisibleSeries(prev => ({
      ...prev,
      [seriesKey]: !prev[seriesKey]
    }));
  };

  // Color scheme for different zones
  const colors = {
    burner: '#e74c3c',   // Red for burner
    zone_1: '#3498db',   // Blue
    zone_2: '#2ecc71',   // Green
    zone_3: '#f39c12',   // Orange
    zone_4: '#9b59b6',   // Purple
    zone_5: '#1abc9c',   // Turquoise
    zone_6: '#e67e22',   // Dark orange
  };

  // Prepare labels (timestamps)
  const labels = data.map(item => {
    const date = new Date(item.timestamp);
    return date.toLocaleDateString() === new Date().toLocaleDateString() 
      ? date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
      : date.toLocaleString([], { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit' });
  });

  // Define all possible datasets
  const allDatasets = [
    {
      key: 'burner',
      label: 'Burner',
      data: data.map(item => item.burner),
      borderColor: colors.burner,
      backgroundColor: colors.burner + '20',
      borderWidth: 3,
      fill: false,
      tension: 0.1,
    },
    {
      key: 'zone_1',
      label: 'Zone 1',
      data: data.map(item => item.zone_1),
      borderColor: colors.zone_1,
      backgroundColor: colors.zone_1 + '20',
      borderWidth: 2,
      fill: false,
      tension: 0.1,
    },
    {
      key: 'zone_2',
      label: 'Zone 2',
      data: data.map(item => item.zone_2),
      borderColor: colors.zone_2,
      backgroundColor: colors.zone_2 + '20',
      borderWidth: 2,
      fill: false,
      tension: 0.1,
    },
    {
      key: 'zone_3',
      label: 'Zone 3',
      data: data.map(item => item.zone_3),
      borderColor: colors.zone_3,
      backgroundColor: colors.zone_3 + '20',
      borderWidth: 2,
      fill: false,
      tension: 0.1,
    },
    {
      key: 'zone_4',
      label: 'Zone 4',
      data: data.map(item => item.zone_4),
      borderColor: colors.zone_4,
      backgroundColor: colors.zone_4 + '20',
      borderWidth: 2,
      fill: false,
      tension: 0.1,
    },
    {
      key: 'zone_5',
      label: 'Zone 5',
      data: data.map(item => item.zone_5),
      borderColor: colors.zone_5,
      backgroundColor: colors.zone_5 + '20',
      borderWidth: 2,
      fill: false,
      tension: 0.1,
    },
    {
      key: 'zone_6',
      label: 'Zone 6',
      data: data.map(item => item.zone_6),
      borderColor: colors.zone_6,
      backgroundColor: colors.zone_6 + '20',
      borderWidth: 2,
      fill: false,
      tension: 0.1,
    },
  ];

  // Filter datasets based on visibility
  const datasets = allDatasets.filter(dataset => visibleSeries[dataset.key]);

  const chartData = {
    labels,
    datasets,
  };

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        display: false,
      },
      title: {
        display: false,
      },
      tooltip: {
        mode: 'index',
        intersect: false,
        callbacks: {
          label: function(context) {
            return `${context.dataset.label}: ${context.parsed.y.toFixed(1)}%`;
          },
        },
      },
    },
    scales: {
      x: {
        display: true,
        title: {
          display: true,
          text: 'Time',
          font: {
            size: 14,
            weight: 'bold',
          },
        },
        grid: {
          color: '#e0e0e0',
        },
      },
      y: {
        display: true,
        title: {
          display: true,
          text: 'Utilization (%)',
          font: {
            size: 14,
            weight: 'bold',
          },
        },
        min: 0,
        max: 100,
        grid: {
          color: '#e0e0e0',
        },
        ticks: {
          callback: function(value) {
            return value + '%';
          },
        },
      },
    },
    interaction: {
      mode: 'nearest',
      axis: 'x',
      intersect: false,
    },
    elements: {
      point: {
        radius: 2,
        hoverRadius: 6,
      },
    },
  };

  return (
    <div>
      <div className="chart-toggles">
        {allDatasets.map((dataset) => (
          <label key={dataset.key} className="chart-toggle-item">
            <input
              type="checkbox"
              checked={visibleSeries[dataset.key]}
              onChange={() => toggleSeries(dataset.key)}
              style={{ accentColor: dataset.borderColor }}
            />
            <span 
              className="chart-toggle-label"
              style={{ color: visibleSeries[dataset.key] ? dataset.borderColor : '#999' }}
            >
              {dataset.label}
            </span>
          </label>
        ))}
      </div>
      <div className="chart-container">
        <Line data={chartData} options={options} />
      </div>
    </div>
  );
};

export default UtilizationChart;
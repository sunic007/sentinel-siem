import React, { useEffect, useState } from 'react';
import { Card, Col, Row, Statistic, Typography, Table, Tag } from 'antd';
import {
  AlertOutlined,
  CheckCircleOutlined,
  WarningOutlined,
  ClockCircleOutlined,
} from '@ant-design/icons';
import ReactECharts from 'echarts-for-react';

const { Title } = Typography;

const SecurityPosture: React.FC = () => {
  const [totalEvents, setTotalEvents] = useState(0);
  const [engineStatus, setEngineStatus] = useState<Record<string, unknown>>({});

  useEffect(() => {
    fetch('/api/health')
      .then((r) => r.json())
      .then((data) => {
        setEngineStatus(data.engine || {});
        setTotalEvents(Number(data.engine?.total_events_ingested || 0));
      })
      .catch(() => {});
  }, []);
  const eventsOverTimeOption = {
    tooltip: { trigger: 'axis' },
    xAxis: {
      type: 'category',
      data: ['00:00', '04:00', '08:00', '12:00', '16:00', '20:00'],
      axisLabel: { color: '#888' },
    },
    yAxis: {
      type: 'value',
      axisLabel: { color: '#888' },
      splitLine: { lineStyle: { color: '#303030' } },
    },
    series: [
      {
        name: 'Events',
        type: 'line',
        smooth: true,
        data: [820, 932, 1200, 1534, 1290, 1100],
        areaStyle: { opacity: 0.15 },
        lineStyle: { color: '#1668dc' },
        itemStyle: { color: '#1668dc' },
      },
    ],
    grid: { left: 50, right: 20, top: 20, bottom: 30 },
    backgroundColor: 'transparent',
  };

  const severityOption = {
    tooltip: { trigger: 'item' },
    series: [
      {
        type: 'pie',
        radius: ['40%', '70%'],
        data: [
          { value: 12, name: 'Critical', itemStyle: { color: '#ff4d4f' } },
          { value: 28, name: 'High', itemStyle: { color: '#ff7a45' } },
          { value: 45, name: 'Medium', itemStyle: { color: '#ffc53d' } },
          { value: 89, name: 'Low', itemStyle: { color: '#52c41a' } },
          { value: 156, name: 'Info', itemStyle: { color: '#1668dc' } },
        ],
        label: { color: '#ccc' },
      },
    ],
    backgroundColor: 'transparent',
  };

  const recentNotables = [
    {
      key: '1',
      time: '2024-01-15 14:32:00',
      title: 'Brute Force Attack Detected',
      severity: 'critical',
      status: 'new',
      source: 'auth.log',
    },
    {
      key: '2',
      time: '2024-01-15 14:28:00',
      title: 'Suspicious DNS Query',
      severity: 'high',
      status: 'in_progress',
      source: 'dns_logs',
    },
    {
      key: '3',
      time: '2024-01-15 14:15:00',
      title: 'Unusual Outbound Traffic',
      severity: 'medium',
      status: 'new',
      source: 'firewall',
    },
    {
      key: '4',
      time: '2024-01-15 13:45:00',
      title: 'Failed Login - Service Account',
      severity: 'high',
      status: 'resolved',
      source: 'windows_security',
    },
  ];

  const severityColors: Record<string, string> = {
    critical: 'red',
    high: 'orange',
    medium: 'gold',
    low: 'green',
    info: 'blue',
  };

  const statusColors: Record<string, string> = {
    new: 'red',
    in_progress: 'orange',
    resolved: 'green',
    closed: 'default',
  };

  const columns = [
    { title: 'Time', dataIndex: 'time', key: 'time', width: 180 },
    { title: 'Title', dataIndex: 'title', key: 'title' },
    {
      title: 'Severity',
      dataIndex: 'severity',
      key: 'severity',
      width: 100,
      render: (severity: string) => (
        <Tag color={severityColors[severity]}>{severity.toUpperCase()}</Tag>
      ),
    },
    {
      title: 'Status',
      dataIndex: 'status',
      key: 'status',
      width: 120,
      render: (status: string) => (
        <Tag color={statusColors[status]}>{status.replace('_', ' ').toUpperCase()}</Tag>
      ),
    },
    { title: 'Source', dataIndex: 'source', key: 'source', width: 150 },
  ];

  return (
    <div style={{ padding: 16 }}>
      <Title level={3} style={{ color: '#fff', marginBottom: 24 }}>
        Security Posture Dashboard
      </Title>

      <Row gutter={[16, 16]}>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic
              title="Total Events Ingested"
              value={totalEvents}
              prefix={<ClockCircleOutlined />}
              valueStyle={{ color: '#1668dc' }}
            />
          </Card>
        </Col>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic
              title="Notable Events"
              value={330}
              prefix={<AlertOutlined />}
              valueStyle={{ color: '#ff7a45' }}
            />
          </Card>
        </Col>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic
              title="Open Incidents"
              value={40}
              prefix={<WarningOutlined />}
              valueStyle={{ color: '#ffc53d' }}
            />
          </Card>
        </Col>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic
              title="Resolved Today"
              value={18}
              prefix={<CheckCircleOutlined />}
              valueStyle={{ color: '#52c41a' }}
            />
          </Card>
        </Col>
      </Row>

      <Row gutter={[16, 16]} style={{ marginTop: 16 }}>
        <Col span={16}>
          <Card
            title="Events Over Time"
            style={{ background: '#1a1a1a', border: '1px solid #303030' }}
            headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
          >
            <ReactECharts option={eventsOverTimeOption} style={{ height: 250 }} />
          </Card>
        </Col>
        <Col span={8}>
          <Card
            title="Notable Events by Severity"
            style={{ background: '#1a1a1a', border: '1px solid #303030' }}
            headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
          >
            <ReactECharts option={severityOption} style={{ height: 250 }} />
          </Card>
        </Col>
      </Row>

      <Row style={{ marginTop: 16 }}>
        <Col span={24}>
          <Card
            title="Recent Notable Events"
            style={{ background: '#1a1a1a', border: '1px solid #303030' }}
            headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
          >
            <Table
              columns={columns}
              dataSource={recentNotables}
              pagination={false}
              size="small"
              style={{ background: 'transparent' }}
            />
          </Card>
        </Col>
      </Row>
    </div>
  );
};

export default SecurityPosture;

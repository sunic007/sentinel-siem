import React, { useEffect, useState } from 'react';
import { Card, Tabs, Form, Input, Button, Switch, Table, Typography, Space, Tag } from 'antd';

const { Title } = Typography;

interface IndexData {
  key: string;
  name: string;
  events: number;
  size: string;
  segments: number;
  status: string;
}

const formatBytes = (bytes: number): string => {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  return `${(bytes / (1024 * 1024 * 1024)).toFixed(1)} GB`;
};

const Settings: React.FC = () => {
  const [indexes, setIndexes] = useState<IndexData[]>([]);
  const [health, setHealth] = useState<Record<string, unknown>>({});

  useEffect(() => {
    fetch('/api/indexes')
      .then((r) => r.json())
      .then((data: Array<{name: string; total_events: number; total_size_bytes: number; segment_count: number}>) => {
        setIndexes(
          data.map((idx, i) => ({
            key: String(i),
            name: idx.name,
            events: idx.total_events,
            size: formatBytes(idx.total_size_bytes),
            segments: idx.segment_count,
            status: 'active',
          }))
        );
      })
      .catch(() => {});

    fetch('/api/health')
      .then((r) => r.json())
      .then(setHealth)
      .catch(() => {});
  }, []);

  const inputs = [
    { key: '1', name: 'Syslog UDP', type: 'syslog', port: 514, status: 'running', events: '1.2k/s' },
    { key: '2', name: 'Syslog TCP', type: 'syslog', port: 1514, status: 'running', events: '340/s' },
    { key: '3', name: '/var/log/auth.log', type: 'file_monitor', port: '-', status: 'running', events: '15/s' },
    { key: '4', name: 'Windows Event Collector', type: 'wec', port: 5985, status: 'stopped', events: '0/s' },
  ];

  const items = [
    {
      key: 'indexes',
      label: 'Indexes',
      children: (
        <Table
          dataSource={indexes}
          columns={[
            { title: 'Name', dataIndex: 'name', key: 'name' },
            { title: 'Events', dataIndex: 'events', key: 'events', render: (n: number) => n.toLocaleString() },
            { title: 'Size', dataIndex: 'size', key: 'size' },
            { title: 'Segments', dataIndex: 'segments', key: 'segments' },
            {
              title: 'Status', dataIndex: 'status', key: 'status',
              render: (s: string) => <Tag color={s === 'active' ? 'green' : 'red'}>{s}</Tag>,
            },
          ]}
          pagination={false}
          size="small"
        />
      ),
    },
    {
      key: 'inputs',
      label: 'Data Inputs',
      children: (
        <Table
          dataSource={inputs}
          columns={[
            { title: 'Name', dataIndex: 'name', key: 'name' },
            { title: 'Type', dataIndex: 'type', key: 'type', render: (t: string) => <Tag>{t}</Tag> },
            { title: 'Port', dataIndex: 'port', key: 'port' },
            {
              title: 'Status', dataIndex: 'status', key: 'status',
              render: (s: string) => <Tag color={s === 'running' ? 'green' : 'red'}>{s}</Tag>,
            },
            { title: 'Rate', dataIndex: 'events', key: 'events' },
          ]}
          pagination={false}
          size="small"
        />
      ),
    },
    {
      key: 'general',
      label: 'General',
      children: (
        <Form layout="vertical" style={{ maxWidth: 500 }}>
          <Form.Item label="Engine Host">
            <Input defaultValue="localhost" />
          </Form.Item>
          <Form.Item label="Engine gRPC Port">
            <Input defaultValue="50051" />
          </Form.Item>
          <Form.Item label="Data Directory">
            <Input defaultValue="./data" />
          </Form.Item>
          <Form.Item label="Enable Dark Mode">
            <Switch defaultChecked />
          </Form.Item>
          <Form.Item>
            <Button type="primary">Save Settings</Button>
          </Form.Item>
        </Form>
      ),
    },
  ];

  return (
    <div style={{ padding: 16 }}>
      <Title level={3} style={{ color: '#fff', marginBottom: 16 }}>Settings</Title>
      <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
        <Tabs items={items} />
      </Card>
    </div>
  );
};

export default Settings;

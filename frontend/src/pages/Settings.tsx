import React from 'react';
import { Card, Tabs, Form, Input, Button, Switch, Table, Typography, Space, Tag } from 'antd';

const { Title } = Typography;

const Settings: React.FC = () => {
  const indexes = [
    { key: '1', name: 'main', events: 1250000, size: '2.4 GB', retention: '90 days', status: 'active' },
    { key: '2', name: 'security', events: 890000, size: '1.8 GB', retention: '365 days', status: 'active' },
    { key: '3', name: 'firewall', events: 3400000, size: '5.1 GB', retention: '30 days', status: 'active' },
    { key: '4', name: 'windows', events: 560000, size: '1.1 GB', retention: '90 days', status: 'active' },
  ];

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
            { title: 'Retention', dataIndex: 'retention', key: 'retention' },
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

import React from 'react';
import { Card, Table, Tag, Typography, Space, Button, Select, Badge } from 'antd';
import { PlusOutlined, ReloadOutlined } from '@ant-design/icons';

const { Title } = Typography;

const IncidentReview: React.FC = () => {
  const incidents = [
    {
      key: '1',
      id: 'INC-001',
      title: 'Brute Force Attack on SSH',
      severity: 'critical',
      status: 'new',
      assignee: null,
      created: '2024-01-15 14:32:00',
      events: 156,
    },
    {
      key: '2',
      id: 'INC-002',
      title: 'Suspicious PowerShell Execution',
      severity: 'high',
      status: 'in_progress',
      assignee: 'analyst1',
      created: '2024-01-15 13:15:00',
      events: 23,
    },
    {
      key: '3',
      id: 'INC-003',
      title: 'Data Exfiltration Attempt',
      severity: 'high',
      status: 'in_progress',
      assignee: 'analyst2',
      created: '2024-01-15 12:45:00',
      events: 89,
    },
    {
      key: '4',
      id: 'INC-004',
      title: 'Malware Detected on Endpoint',
      severity: 'medium',
      status: 'resolved',
      assignee: 'analyst1',
      created: '2024-01-15 10:20:00',
      events: 12,
    },
  ];

  const severityColors: Record<string, string> = {
    critical: 'red',
    high: 'orange',
    medium: 'gold',
    low: 'green',
  };

  const statusColors: Record<string, string> = {
    new: 'red',
    in_progress: 'blue',
    resolved: 'green',
    closed: 'default',
  };

  const columns = [
    { title: 'ID', dataIndex: 'id', key: 'id', width: 100 },
    { title: 'Title', dataIndex: 'title', key: 'title' },
    {
      title: 'Severity',
      dataIndex: 'severity',
      key: 'severity',
      width: 100,
      render: (s: string) => <Tag color={severityColors[s]}>{s.toUpperCase()}</Tag>,
    },
    {
      title: 'Status',
      dataIndex: 'status',
      key: 'status',
      width: 130,
      render: (s: string) => (
        <Badge
          status={s === 'new' ? 'error' : s === 'in_progress' ? 'processing' : 'success'}
          text={<Tag color={statusColors[s]}>{s.replace('_', ' ').toUpperCase()}</Tag>}
        />
      ),
    },
    {
      title: 'Assignee',
      dataIndex: 'assignee',
      key: 'assignee',
      width: 120,
      render: (a: string | null) => a || <Tag>Unassigned</Tag>,
    },
    { title: 'Created', dataIndex: 'created', key: 'created', width: 180 },
    {
      title: 'Events',
      dataIndex: 'events',
      key: 'events',
      width: 80,
      render: (n: number) => <Tag>{n}</Tag>,
    },
  ];

  return (
    <div style={{ padding: 16 }}>
      <Space style={{ width: '100%', justifyContent: 'space-between', marginBottom: 16 }}>
        <Title level={3} style={{ color: '#fff', margin: 0 }}>
          Incident Review
        </Title>
        <Space>
          <Select
            defaultValue="all"
            style={{ width: 150 }}
            options={[
              { value: 'all', label: 'All Statuses' },
              { value: 'new', label: 'New' },
              { value: 'in_progress', label: 'In Progress' },
              { value: 'resolved', label: 'Resolved' },
              { value: 'closed', label: 'Closed' },
            ]}
          />
          <Select
            defaultValue="all"
            style={{ width: 150 }}
            options={[
              { value: 'all', label: 'All Severities' },
              { value: 'critical', label: 'Critical' },
              { value: 'high', label: 'High' },
              { value: 'medium', label: 'Medium' },
              { value: 'low', label: 'Low' },
            ]}
          />
          <Button icon={<ReloadOutlined />}>Refresh</Button>
          <Button type="primary" icon={<PlusOutlined />}>
            Create Incident
          </Button>
        </Space>
      </Space>

      <Card
        style={{ background: '#1a1a1a', border: '1px solid #303030' }}
        bodyStyle={{ padding: 0 }}
      >
        <Table
          columns={columns}
          dataSource={incidents}
          pagination={{ pageSize: 20 }}
          size="small"
          onRow={() => ({
            style: { cursor: 'pointer' },
          })}
        />
      </Card>
    </div>
  );
};

export default IncidentReview;

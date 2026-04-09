import React from 'react';
import { Card, Table, Tag, Typography, Input, Space, Button } from 'antd';
import { UploadOutlined, PlusOutlined } from '@ant-design/icons';

const { Title } = Typography;
const { Search: SearchInput } = Input;

const AssetCenter: React.FC = () => {
  const assets = [
    { key: '1', hostname: 'web-server-01', ip: '10.0.1.10', os: 'Ubuntu 22.04', criticality: 'high', unit: 'Engineering', owner: 'admin' },
    { key: '2', hostname: 'db-server-01', ip: '10.0.2.20', os: 'CentOS 8', criticality: 'critical', unit: 'Database', owner: 'dba-team' },
    { key: '3', hostname: 'fw-edge-01', ip: '10.0.0.1', os: 'PAN-OS 11', criticality: 'critical', unit: 'Network', owner: 'netops' },
    { key: '4', hostname: 'dc-01', ip: '10.0.1.5', os: 'Windows Server 2022', criticality: 'critical', unit: 'IT', owner: 'ad-team' },
    { key: '5', hostname: 'laptop-user01', ip: '10.0.10.101', os: 'Windows 11', criticality: 'low', unit: 'Sales', owner: 'user01' },
  ];

  const criticalityColors: Record<string, string> = {
    critical: 'red', high: 'orange', medium: 'gold', low: 'green',
  };

  return (
    <div style={{ padding: 16 }}>
      <Space style={{ width: '100%', justifyContent: 'space-between', marginBottom: 16 }}>
        <Title level={3} style={{ color: '#fff', margin: 0 }}>Asset Center</Title>
        <Space>
          <SearchInput placeholder="Search assets..." style={{ width: 300 }} />
          <Button icon={<UploadOutlined />}>Import CSV</Button>
          <Button type="primary" icon={<PlusOutlined />}>Add Asset</Button>
        </Space>
      </Space>

      <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }} bodyStyle={{ padding: 0 }}>
        <Table
          dataSource={assets}
          columns={[
            { title: 'Hostname', dataIndex: 'hostname', key: 'hostname' },
            { title: 'IP Address', dataIndex: 'ip', key: 'ip' },
            { title: 'OS', dataIndex: 'os', key: 'os' },
            {
              title: 'Criticality', dataIndex: 'criticality', key: 'criticality',
              render: (c: string) => <Tag color={criticalityColors[c]}>{c.toUpperCase()}</Tag>,
            },
            { title: 'Business Unit', dataIndex: 'unit', key: 'unit' },
            { title: 'Owner', dataIndex: 'owner', key: 'owner' },
          ]}
          pagination={{ pageSize: 20 }}
          size="small"
        />
      </Card>
    </div>
  );
};

export default AssetCenter;

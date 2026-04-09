import React from 'react';
import { Card, Col, Row, Statistic, Table, Tag, Typography, Button } from 'antd';
import { PlusOutlined, SyncOutlined } from '@ant-design/icons';

const { Title } = Typography;

const ThreatIntelligence: React.FC = () => {
  const feeds = [
    { key: '1', name: 'AlienVault OTX', type: 'STIX/TAXII', status: 'active', iocs: 45230, lastPoll: '5 min ago' },
    { key: '2', name: 'Abuse.ch URLhaus', type: 'CSV', status: 'active', iocs: 12800, lastPoll: '15 min ago' },
    { key: '3', name: 'MISP Community', type: 'STIX/TAXII', status: 'error', iocs: 0, lastPoll: '2h ago' },
  ];

  const recentMatches = [
    { key: '1', type: 'IP', value: '185.220.101.34', source: 'AlienVault OTX', matchedIn: 'firewall', time: '2 min ago' },
    { key: '2', type: 'Domain', value: 'evil-domain.xyz', source: 'URLhaus', matchedIn: 'dns_logs', time: '8 min ago' },
    { key: '3', type: 'Hash', value: 'a1b2c3d4...', source: 'MISP', matchedIn: 'endpoint', time: '1h ago' },
  ];

  return (
    <div style={{ padding: 16 }}>
      <Title level={3} style={{ color: '#fff', marginBottom: 24 }}>
        Threat Intelligence
      </Title>

      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic title="Total IOCs" value={58030} valueStyle={{ color: '#1668dc' }} />
          </Card>
        </Col>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic title="Active Feeds" value={2} valueStyle={{ color: '#52c41a' }} />
          </Card>
        </Col>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic title="Matches (24h)" value={47} valueStyle={{ color: '#ff7a45' }} />
          </Card>
        </Col>
        <Col span={6}>
          <Card style={{ background: '#1a1a1a', border: '1px solid #303030' }}>
            <Statistic title="New IOCs Today" value={1250} valueStyle={{ color: '#ffc53d' }} />
          </Card>
        </Col>
      </Row>

      <Card
        title="Threat Feeds"
        extra={<Button type="primary" icon={<PlusOutlined />}>Add Feed</Button>}
        style={{ background: '#1a1a1a', border: '1px solid #303030', marginBottom: 16 }}
        headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
      >
        <Table
          dataSource={feeds}
          columns={[
            { title: 'Feed Name', dataIndex: 'name', key: 'name' },
            { title: 'Type', dataIndex: 'type', key: 'type' },
            {
              title: 'Status',
              dataIndex: 'status',
              key: 'status',
              render: (s: string) => (
                <Tag color={s === 'active' ? 'green' : 'red'}>
                  {s === 'active' ? <SyncOutlined spin /> : null} {s.toUpperCase()}
                </Tag>
              ),
            },
            { title: 'IOCs', dataIndex: 'iocs', key: 'iocs', render: (n: number) => n.toLocaleString() },
            { title: 'Last Poll', dataIndex: 'lastPoll', key: 'lastPoll' },
          ]}
          pagination={false}
          size="small"
        />
      </Card>

      <Card
        title="Recent IOC Matches"
        style={{ background: '#1a1a1a', border: '1px solid #303030' }}
        headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
      >
        <Table
          dataSource={recentMatches}
          columns={[
            { title: 'Type', dataIndex: 'type', key: 'type', render: (t: string) => <Tag>{t}</Tag> },
            { title: 'Value', dataIndex: 'value', key: 'value' },
            { title: 'Source', dataIndex: 'source', key: 'source' },
            { title: 'Matched In', dataIndex: 'matchedIn', key: 'matchedIn' },
            { title: 'Time', dataIndex: 'time', key: 'time' },
          ]}
          pagination={false}
          size="small"
        />
      </Card>
    </div>
  );
};

export default ThreatIntelligence;

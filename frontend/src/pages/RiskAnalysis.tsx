import React from 'react';
import { Card, Col, Row, Table, Tag, Typography, Progress } from 'antd';
import ReactECharts from 'echarts-for-react';

const { Title } = Typography;

const RiskAnalysis: React.FC = () => {
  const riskEntities = [
    { key: '1', entity: 'user_admin', type: 'user', score: 87, factors: 5, status: 'critical' },
    { key: '2', entity: '10.0.1.10', type: 'host', score: 65, factors: 3, status: 'high' },
    { key: '3', entity: 'svc_backup', type: 'user', score: 42, factors: 2, status: 'medium' },
    { key: '4', entity: 'db-server-01', type: 'host', score: 35, factors: 2, status: 'medium' },
    { key: '5', entity: 'user_jsmith', type: 'user', score: 15, factors: 1, status: 'low' },
  ];

  const riskTrendOption = {
    tooltip: { trigger: 'axis' },
    legend: { data: ['Critical', 'High', 'Medium'], textStyle: { color: '#888' } },
    xAxis: {
      type: 'category',
      data: ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'],
      axisLabel: { color: '#888' },
    },
    yAxis: { type: 'value', axisLabel: { color: '#888' }, splitLine: { lineStyle: { color: '#303030' } } },
    series: [
      { name: 'Critical', type: 'line', data: [2, 3, 1, 4, 2, 1, 3], itemStyle: { color: '#ff4d4f' } },
      { name: 'High', type: 'line', data: [5, 7, 4, 8, 6, 3, 5], itemStyle: { color: '#ff7a45' } },
      { name: 'Medium', type: 'line', data: [12, 15, 10, 18, 14, 8, 11], itemStyle: { color: '#ffc53d' } },
    ],
    grid: { left: 40, right: 20, top: 40, bottom: 30 },
    backgroundColor: 'transparent',
  };

  const statusColors: Record<string, string> = {
    critical: 'red', high: 'orange', medium: 'gold', low: 'green',
  };

  return (
    <div style={{ padding: 16 }}>
      <Title level={3} style={{ color: '#fff', marginBottom: 24 }}>Risk Analysis</Title>

      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        <Col span={16}>
          <Card
            title="Risk Score Trend (7 Days)"
            style={{ background: '#1a1a1a', border: '1px solid #303030' }}
            headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
          >
            <ReactECharts option={riskTrendOption} style={{ height: 250 }} />
          </Card>
        </Col>
        <Col span={8}>
          <Card
            title="Risk Distribution"
            style={{ background: '#1a1a1a', border: '1px solid #303030' }}
            headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
          >
            <div style={{ padding: '16px 0' }}>
              <div style={{ marginBottom: 16 }}>
                <span style={{ color: '#ccc' }}>Critical (&gt;75)</span>
                <Progress percent={8} strokeColor="#ff4d4f" trailColor="#303030" />
              </div>
              <div style={{ marginBottom: 16 }}>
                <span style={{ color: '#ccc' }}>High (50-75)</span>
                <Progress percent={15} strokeColor="#ff7a45" trailColor="#303030" />
              </div>
              <div style={{ marginBottom: 16 }}>
                <span style={{ color: '#ccc' }}>Medium (25-50)</span>
                <Progress percent={32} strokeColor="#ffc53d" trailColor="#303030" />
              </div>
              <div>
                <span style={{ color: '#ccc' }}>Low (&lt;25)</span>
                <Progress percent={45} strokeColor="#52c41a" trailColor="#303030" />
              </div>
            </div>
          </Card>
        </Col>
      </Row>

      <Card
        title="Top Risk Entities"
        style={{ background: '#1a1a1a', border: '1px solid #303030' }}
        headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
      >
        <Table
          dataSource={riskEntities}
          columns={[
            { title: 'Entity', dataIndex: 'entity', key: 'entity' },
            { title: 'Type', dataIndex: 'type', key: 'type', render: (t: string) => <Tag>{t}</Tag> },
            {
              title: 'Risk Score', dataIndex: 'score', key: 'score', width: 200,
              render: (s: number) => (
                <Progress
                  percent={s}
                  strokeColor={s > 75 ? '#ff4d4f' : s > 50 ? '#ff7a45' : s > 25 ? '#ffc53d' : '#52c41a'}
                  trailColor="#303030"
                  size="small"
                />
              ),
            },
            { title: 'Risk Factors', dataIndex: 'factors', key: 'factors' },
            {
              title: 'Status', dataIndex: 'status', key: 'status',
              render: (s: string) => <Tag color={statusColors[s]}>{s.toUpperCase()}</Tag>,
            },
          ]}
          pagination={false}
          size="small"
        />
      </Card>
    </div>
  );
};

export default RiskAnalysis;

import React, { useState, useCallback } from 'react';
import { Button, Card, Table, Typography, Space, Select, DatePicker, Tag } from 'antd';
import { SearchOutlined, PlayCircleOutlined } from '@ant-design/icons';
import Editor from '@monaco-editor/react';
import ReactECharts from 'echarts-for-react';

const { Title, Text } = Typography;
const { RangePicker } = DatePicker;

interface SearchEvent {
  key: string;
  time: string;
  raw: string;
  host: string;
  source: string;
  sourcetype: string;
}

const Search: React.FC = () => {
  const [query, setQuery] = useState('search index=main | head 100');
  const [results, setResults] = useState<SearchEvent[]>([]);
  const [loading, setLoading] = useState(false);
  const [executionTime, setExecutionTime] = useState(0);

  const handleSearch = useCallback(async () => {
    setLoading(true);
    try {
      const response = await fetch('/api/search/execute', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ query }),
      });
      const data = await response.json();
      setResults(
        (data.events || []).map((e: SearchEvent, i: number) => ({
          ...e,
          key: String(i),
        }))
      );
      setExecutionTime(data.execution_time_ms || 0);
    } catch {
      setResults([]);
    } finally {
      setLoading(false);
    }
  }, [query]);

  const handleEditorChange = (value: string | undefined) => {
    if (value !== undefined) setQuery(value);
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
      handleSearch();
    }
  };

  const timelineOption = {
    tooltip: { trigger: 'axis' },
    xAxis: {
      type: 'category',
      data: results.length > 0
        ? results.slice(0, 20).map((e) => e.time?.substring(11, 19) || '')
        : ['No data'],
      axisLabel: { color: '#888', rotate: 45 },
    },
    yAxis: {
      type: 'value',
      axisLabel: { color: '#888' },
      splitLine: { lineStyle: { color: '#303030' } },
    },
    series: [
      {
        type: 'bar',
        data: results.length > 0 ? results.slice(0, 20).map(() => 1) : [0],
        itemStyle: { color: '#1668dc' },
      },
    ],
    grid: { left: 40, right: 10, top: 10, bottom: 60 },
    backgroundColor: 'transparent',
  };

  const columns = [
    {
      title: 'Time',
      dataIndex: 'time',
      key: 'time',
      width: 180,
      ellipsis: true,
    },
    {
      title: 'Host',
      dataIndex: 'host',
      key: 'host',
      width: 150,
      render: (host: string) => <Tag>{host}</Tag>,
    },
    {
      title: 'Source',
      dataIndex: 'source',
      key: 'source',
      width: 200,
      ellipsis: true,
    },
    {
      title: 'Raw Event',
      dataIndex: 'raw',
      key: 'raw',
      ellipsis: true,
    },
  ];

  return (
    <div style={{ padding: 16 }} onKeyDown={handleKeyDown}>
      <Title level={3} style={{ color: '#fff', marginBottom: 16 }}>
        Search
      </Title>

      {/* SPL Editor */}
      <Card
        style={{
          background: '#1a1a1a',
          border: '1px solid #303030',
          marginBottom: 16,
        }}
        bodyStyle={{ padding: 0 }}
      >
        <div style={{ borderBottom: '1px solid #303030', padding: '8px 16px' }}>
          <Space>
            <Select
              defaultValue="main"
              style={{ width: 150 }}
              options={[
                { value: '*', label: 'All Indexes' },
                { value: 'main', label: 'main' },
                { value: 'security', label: 'security' },
                { value: 'firewall', label: 'firewall' },
                { value: 'windows', label: 'windows' },
              ]}
            />
            <RangePicker showTime style={{ width: 380 }} />
            <Button
              type="primary"
              icon={<PlayCircleOutlined />}
              onClick={handleSearch}
              loading={loading}
            >
              Search
            </Button>
            <Text type="secondary" style={{ marginLeft: 8 }}>
              Ctrl+Enter to search
            </Text>
          </Space>
        </div>
        <Editor
          height="120px"
          defaultLanguage="plaintext"
          theme="vs-dark"
          value={query}
          onChange={handleEditorChange}
          options={{
            minimap: { enabled: false },
            scrollBeyondLastLine: false,
            lineNumbers: 'off',
            glyphMargin: false,
            folding: false,
            fontSize: 14,
            fontFamily: '"Fira Code", "Cascadia Code", Consolas, monospace',
            wordWrap: 'on',
            padding: { top: 8, bottom: 8 },
          }}
        />
      </Card>

      {/* Results */}
      {executionTime > 0 && (
        <Text type="secondary" style={{ display: 'block', marginBottom: 8 }}>
          {results.length} results in {executionTime.toFixed(2)}ms
        </Text>
      )}

      {/* Event Timeline */}
      <Card
        title="Event Timeline"
        size="small"
        style={{
          background: '#1a1a1a',
          border: '1px solid #303030',
          marginBottom: 16,
        }}
        headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
      >
        <ReactECharts option={timelineOption} style={{ height: 150 }} />
      </Card>

      {/* Results Table */}
      <Card
        title="Events"
        size="small"
        style={{ background: '#1a1a1a', border: '1px solid #303030' }}
        headStyle={{ color: '#fff', borderBottom: '1px solid #303030' }}
      >
        <Table
          columns={columns}
          dataSource={results}
          loading={loading}
          pagination={{ pageSize: 50, showSizeChanger: true }}
          size="small"
          scroll={{ y: 400 }}
          locale={{ emptyText: 'Run a search to see results' }}
        />
      </Card>
    </div>
  );
};

export default Search;
